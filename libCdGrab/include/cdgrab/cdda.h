#pragma once

#include <cdgrab/gpc.h>
#include <cdgrab/grab_export.h>


typedef std::array<uint8_t, (int)CddaFrame::Audio> AudioFrame;

GRAB_EXPORT std::vector<std::string> GetDevices();

/// Implementation of the Generic Packet Transport.
/// Some of the calls can be done directly to the OS-provided functions,
/// instead of going over the GPC protocol
class GRAB_EXPORT CDDA :
	public GenericPacket,
	private GpcTransport
{
public:
	enum class CommunicationChannel
	{
		MMC, SCSI
	};

	enum class DriveStatus
	{
		Error = -1,
		NoInfo = 0,
		NoDisc = 1,
		TrayOpen = 2,
		NotReady = 3,
		DiscOk = 4
	};

	CDDA(std::string deviceName, CommunicationChannel channel = CommunicationChannel::SCSI);

	~CDDA() override;

	// Implement GpcTransport
	int GenericPacketCall(buffer_view<const uint8_t> cmd, buffer_view<uint8_t> data) override;

	DriveStatus GetDriveStatus();

	DiscStatus GetDiscStatus() override;

	// Relative to last call
	bool HasMediaChanged();

	/// Returns 98 frames of audio (no subframe)
	void ReadSector(MSF pos, std::array<char, (int)CddaFrame::Audio>& dst);

	/// Read nrFrames*CD_FRAMESIZE_RAW bytes
	/// There might be a max. of 25 frames
	int ReadAudio(int LBA, buffer_view<int16_t> dst, int nrFrames) override;

	// TODO: Overload these, call GenericPacket by default.
	// allows per-OS implementation
	//Toc GetTOCHL();
	//int SizeInSectorsHl();
	//std::string UniversalProductCodeHL();

	void Eject();

private:
	struct Impl;
	std::unique_ptr<Impl> m;
};

std::ostream& operator<<(std::ostream& os, CDDA::DriveStatus s);
