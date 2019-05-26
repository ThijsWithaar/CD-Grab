#pragma once

#include <array>
#include <map>
#include <set>
#include <vector>

#include <boost/format.hpp>
#include <boost/serialization/version.hpp>

#include <cdgrab/buffer_view.h>
#include <cdgrab/grab_export.h>


namespace CddaAddress
{
	const static int sec_minute = 60;
	const static int frames_sec = 75;
	const static int frames_sector = 98;
}

/// Original time-stamp of frames
struct MSF
{
	int minute;
	int second;
	int frame;
};

/// Convert to absolute frame number
GRAB_EXPORT int ToFrame(MSF msf);

GRAB_EXPORT MSF FromFrame(int frame);

/// Convert to sector number ??
int ToLBA(MSF msf);

/// Convert sector number (LBA) to MSF time-stamp
GRAB_EXPORT MSF ToMSF(int lba);

// Page 353, Table 279
enum class CddaFrame
{
	QsubChannel = 16,		///< Q sub-channel, decoded
	RawSubChannel = 96,		///< Raw sub-channel, not decoded
	C2pointers = 294,		///< Bitmask of all per-byte errors
	C2block = 2,			///< Logical or of all c2-pointer bytes
	Audio = 2352,			///< Audio data, as signed-16bit, interleaved
	AudioError = (int)Audio + (int)C2pointers,
	AudioBlockError = (int)AudioError + (int)C2block,
	AudioBlockErrorSub = (int)AudioBlockError + RawSubChannel
};

enum class TocControl
{
	PreEmphasis = 1,
	CopyPermitted = 2,
	DataTrack = 4,
	FourChannel = 8
};

/// See table 343, page 395
enum class TocAdr
{
	NoInfo = 0,
	CurrentPosition = 1,
	MCN = 2,
	ISRC = 3
};

struct TocEntry: public MSF
{
	int track;					///< Track number
	TocAdr adr;					///< Which information is in the Q sub-channel of this block
	TocControl control;
	std::array<char,12> isrc;	///< International Standard Recording Code
	std::string title;			///< Track title
	std::string performer;
};
BOOST_CLASS_VERSION(TocEntry, 2)


struct Toc
{
	std::string upc;	///< Universal Product Code / Media Catalogus Number
	std::string title;	///< Album title
	std::string performer;
	std::vector<TocEntry> tracks;
};
BOOST_CLASS_VERSION(Toc, 1)


enum class EventNotificationClass
{
	NotSupported = 0,
	OperationalChane = 0x01,
	PowerManagement = 0x02,
	ExternalRequest = 0x03,
	Media = 0x04,
	MultiHost = 0x05,
	DeviceBusy = 0x06,
	Reserved = 0x07
};

struct Capabilities
{
	bool UPC;
	bool ISRC;
	bool C2pointers;		///< Error correction
	bool streamAccurate;
	int nrVolumeLevels;
};

template<typename T>
void CheckSerializationVersion(unsigned int version)
{
	const unsigned vcode = boost::serialization::version<T>::value;
	if(vcode != version)
		throw std::runtime_error((boost::format("Serialization version mismatch for '%s'.\nVersion %i is not supported, required version is %i.")
		% typeid(T).name() % version % vcode).str());
}

/// Interface for the low-level Generic Packet Call.
/// Windows has one way, linux has two (SCSI and MMC).
/// This provides an interface for this, so the rest of this code is independent of the transport.
/// The os-dependent part should implement this, to enable GenericPacket
class GpcTransport
{
public:
	virtual int GenericPacketCall(buffer_view<const uint8_t> cmd, buffer_view<uint8_t> data) = 0;
};

/// Interface for reading audio, no subframes or error correction
class AudioReader
{
public:
	struct HardwareInfo
	{
		std::string vendor, model, revision;
	};

	virtual ~AudioReader() { }

	/// Identification of the reader hardware
	virtual HardwareInfo GetHardwareInfo() = 0;

	/// Read the Table-Of-Contents, optionally scan for ISRC and CD-TEXT as well.
	virtual Toc GetToc(bool isrc=true, bool cdtext=true) = 0;

	/// Size in sectors of the current disc
	virtual int SizeInSectors() = 0;

	/// Read nrFrames*CddaFrame::Audio samples
	virtual int ReadAudio(int LBA, buffer_view<int16_t> dst, int nrFrames) = 0;
};

BOOST_CLASS_VERSION(AudioReader::HardwareInfo, 0)


/// This class is intended to be wrapped by another, which implements GpcTransport
/// The Generic Packet is a low-level abstraction of all CD-ROM drives.
/// It provides more functionality than the higher level Linux or Windows IOCTL's
class GRAB_EXPORT GenericPacket : public AudioReader
{
public:
	struct PerfomanceInfo
	{
		int read_kBs;
		int write_kBs;
	};

	enum class DiscStatus
	{
		Error = -1,
		NoInfo = 0,
		NoDisc = 1,
		Audio = 100,
		Data1 = 101,
		Data2 = 102,
		XA1 = 103,
		XA2 = 104,
		Mixed = 105
	};

	/// Transport has to exist for the livetime of this GenericPacket instance
	GenericPacket(GpcTransport* transport);

	HardwareInfo GetHardwareInfo() override;

	Capabilities GetCapabilities();

	PerfomanceInfo GetPerformance();

	/// Hardware Configuration
	void GetConfiguration();

	int UnitReady();

	/// Currently insterted medium
	virtual DiscStatus GetDiscStatus();

	/// Returns lowest possible performance
	PerfomanceInfo GetMinimumWriteSpeed();

	Toc GetToc(bool isrc, bool cdtext) override;

	/// Index of last sector (LBA)
	int SizeInSectors() final;

	/// Request event.
	/// Blocking can be used to block for the next event,
	/// in combination with a time-out on the `GpcTransport`
	void GetStatus(std::set<EventNotificationClass> mask, bool blocking=false);

	/// Reads the blocks, returns sum of E2 checks
	int ReadCd(int lba, int nrBlocks, bool);

	/// Probably only the write cache
	void FlushCache();

protected:
	struct CdText
	{
		std::map<int, std::string> titles, performer;
	};

	/// UPC aka Media Catalog Number. returns both block index and data itself
	std::pair<int, std::string> UniversalProductCode();

	/// International Standard Recording Code (ISRC)
	/// GetToc() fetches this
	std::array<char,12> GetISRC(int track);

	/// Returns title per track (index 0 for album)
	CdText GetCdText();

private:
	GpcTransport* m_io;

	// Max. size of CCDA buffer
	std::array<uint8_t, (int)CddaFrame::AudioBlockErrorSub> m_buf;
};

GRAB_EXPORT std::ostream& operator<<(std::ostream& os, GenericPacket::DiscStatus s);
