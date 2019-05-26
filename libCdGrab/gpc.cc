#include <cdgrab/gpc.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>

#include <boost/format.hpp>

#include "MtFuji/cdtext.h"
#include "MtFuji/configuration.h"
#include "MtFuji/discInformation.h"
#include "parse.h"

#ifdef _WIN32
#include <intrin.h>
#define __builtin_popcount __popcnt16
#endif


enum class GenericPacketCommand
{
	TestUnitReady = 0x00,
	ModeSense = 0x5A,
	Performance = 0xAC,
	WriteSpeed = 0x03,
	FlushCache = 0x35,
	Inquiry = 0x12,
	WriteBuffer = 0x3B,
	ReadBuffer = 0x3C,
	ReadSubChannel = 0x42,
	ReadToc = 0x43,
	ReadHeader = 0x44,
	GetConfiguration = 0x46,
	GetEvent = 0x4A,
	DiscInformation = 0x51,
	ReadCD = 0xBE
};


enum class AddressType
{
	LBA = 0x01,
	MSF = 0x02
};


enum class TrackId
{
	LeadOut = 0xAA
};


/// Page 402, Section 13.29, Table 351
enum class ReadToc
{
	Toc = 0,
	Session = 1,
	FullToc = 2,
	Pma = 3,  ///< Q subcode data
	Atip = 4, ///< Media type
	CdText = 5
};


enum class SubchannelTrack
{
	Toc = 1,
	MediaCatalogNumber = 2,
	ISRC = 3
};


int ToFrame(MSF msf)
{
	return (msf.minute * CddaAddress::sec_minute + msf.second) * CddaAddress::frames_sec +
		   msf.frame;
}


MSF FromFrame(int frame)
{
	MSF r;
	r.frame = frame % CddaAddress::frames_sec;
	r.second = frame / CddaAddress::frames_sec;
	r.minute = r.second / CddaAddress::sec_minute;
	r.second %= r.second;
	return r;
}


/// Convert to sector number
int ToLBA(MSF msf)
{
	return ToFrame(msf) / CddaAddress::frames_sector;
}


MSF ToMSF(int lba)
{
	MSF msf;
	msf.frame = lba % CddaAddress::frames_sec;
	lba /= CddaAddress::frames_sec;
	msf.second = lba % CddaAddress::sec_minute;
	lba /= CddaAddress::sec_minute;
	msf.minute = lba;
	return msf;
}


// Command Descriptor buffer
class CDB
{
public:
	CDB();

	void SetCommand(GenericPacketCommand cmd);
	void SetReadType(uint8_t sectorType);
	void SetLen(int pos, uint16_t len);
	void SetStartTrack(uint8_t cmd);
	void SetMainChannelSelectionBits(uint8_t val);
	void SetReadLength8(uint8_t len);
	void SetReadLength16(uint16_t len);
	void SetReadLength(uint32_t len);

	std::array<uint8_t, 12>& data();

	buffer_view<const uint8_t> view();

private:
	std::array<uint8_t, 12> m;
};


CDB::CDB(): m{0}
{
}


void CDB::SetCommand(GenericPacketCommand cmd)
{
	m[0] = (int)cmd;
}


void CDB::SetReadType(uint8_t sectorType)
{
	m[1] = sectorType << 2;
}


void CDB::SetLen(int pos, uint16_t len)
{
	m[pos] = (len >> 8);
	m[pos + 1] = len & 0xFF;
}


void CDB::SetStartTrack(uint8_t cmd)
{
	m[6] = cmd;
}


void CDB::SetMainChannelSelectionBits(uint8_t val)
{
	m[9] = val << 3;
}


void CDB::SetReadLength8(uint8_t len)
{
	m[8] = len;
}


void CDB::SetReadLength16(uint16_t len)
{
	m[7] = (len >> 8) & 0xFF;
	m[8] = (len)&0xFF;
}


void CDB::SetReadLength(uint32_t len)
{
	m[6] = (len >> 16) & 0xFF;
	m[7] = (len >> 8) & 0xFF;
	m[8] = (len)&0xFF;
}


std::array<uint8_t, 12>& CDB::data()
{
	return m;
}


buffer_view<const uint8_t> CDB::view()
{
	const uint8_t* ptr = m.data();
	return buffer_view<const uint8_t>{ptr, (std::ptrdiff_t)m.size()};
}


GenericPacket::GenericPacket(GpcTransport* transport): m_io(transport)
{
}


GenericPacket::HardwareInfo GenericPacket::GetHardwareInfo()
{
	const int VendorLength = 8;
	const int ModelLength = 16;
	const int RevisionLength = 4;

	std::array<char, 36> buf{0};

	CDB cdb;
	cdb.SetCommand(GenericPacketCommand::Inquiry);
	cdb.data()[4] = static_cast<uint8_t>(buf.size());
	m_io->GenericPacketCall(cdb.view(), {(uint8_t*)buf.data(), (ptrdiff_t)buf.size()});

	auto pVendor = buf.data() + 8;
	auto pModel = pVendor + VendorLength;
	auto pRevision = pModel + ModelLength;

	HardwareInfo info;
	info.vendor.assign(pVendor, pVendor + VendorLength);
	info.model.assign(pModel, pModel + ModelLength);
	info.revision.assign(pRevision, pRevision + RevisionLength);
	return info;
}


enum class ModePageCodes
{
	ErrorRecovery = 0x01,
	FailureReporting = 0x1C,
	CdDvdCapabilities = 0x2A
};


enum class PageControl
{
	Current = 0,
	Changeable = 1,
	Default = 2,
	Saved = 3
};


Capabilities GenericPacket::GetCapabilities()
{
	Capabilities r;

	std::array<char, 36> buf{0};

	CDB cdb;
	cdb.SetCommand(GenericPacketCommand::ModeSense);
	SetBits<3, 1>(cdb.data()[1], 1); // DisableBlockDescriptor
	SetBits<6, 2>(cdb.data()[2], (int)PageControl::Current);
	SetBits<0, 6>(cdb.data()[2], (int)ModePageCodes::CdDvdCapabilities);
	cdb.SetReadLength16(static_cast<uint16_t>(buf.size()));

	m_io->GenericPacketCall(cdb.view(), {(uint8_t*)buf.data(), (ptrdiff_t)buf.size()});

	const char* pHeader = &buf[0];
	const char* pModePage = pHeader + 8;		 // Table 218
	const char* pModeParameters = pModePage + 2; // Table 219

	int modeDataLength = parse<uint16_t>(&pHeader[0]);
	int pageCode = GetBits<0, 6>(pModePage[0]);
	int pageLength = pModePage[1];

	int pc = GetBits<0, 6>(pModePage[0]);
	assert(pc == (int)ModePageCodes::CdDvdCapabilities);
	r.UPC = GetBit<6>(pModePage[5]);
	r.ISRC = GetBit<5>(pModePage[5]);
	r.C2pointers = GetBit<4>(pModePage[5]);
	r.streamAccurate = GetBit<1>(pModePage[5]);

	r.nrVolumeLevels = parse<uint16_t>(&pModePage[10]);

	// std::cout << boost::format("pc = %02x, upc %i, isrc %i, c2 %i, sa %i, ") % pc % (int)r.UPC %
	//				 (int)r.ISRC % (int)r.C2pointers % (int)r.streamAccurate;
	// std::cout << boost::format("nrvol = %i\n") % r.nrVolumeLevels;

	return r;
}


enum class PerformanceType
{
	Performance = 0,
	UnusuableArea = 1,
	DefectStatus = 2,
	WriteSpeed = 3
};


enum class PerformanceDataType
{
	Tolerance = 0x10,
	ReadSpeed = 0x00,
	WriteSpeed = 0x04,
	ExceptNominal = 0x00,
	ExceptAll = 0x01,
};


struct PerformanceDescriptor
{
	uint32_t startLba, endLba;
	uint32_t startPerformance, endPerformance;
};


template<>
PerformanceDescriptor parse(const void* pBuf)
{
	const char* cBuf = reinterpret_cast<const char*>(pBuf);
	// Table 186
	PerformanceDescriptor r;
	r.startLba = parse<uint32_t>(&cBuf[0]);
	r.startPerformance = parse<uint32_t>(&cBuf[4]);
	r.endLba = parse<uint32_t>(&cBuf[8]);
	r.endPerformance = parse<uint32_t>(&cBuf[12]);
	return r;
}


GenericPacket::PerfomanceInfo GenericPacket::GetPerformance()
{
	const int performanceDescriptorSize = 16;
	const int maxNrDescriptors = 4;

	PerfomanceInfo ret;
	std::array<uint8_t, 8 + 16 * maxNrDescriptors> buf;

	CDB cdb;
	cdb.SetCommand(GenericPacketCommand::Performance);
	cdb.data()[1] = (int)PerformanceDataType::Tolerance | (int)PerformanceDataType::ExceptNominal;
	cdb.data()[10] = (int)PerformanceType::Performance;
	cdb.data()[9] = maxNrDescriptors;

	// Read speed
	m_io->GenericPacketCall(cdb.view(), {(uint8_t*)buf.data(), (ptrdiff_t)buf.size()});

	auto rp = parse<PerformanceDescriptor>(&buf[8]);
	ret.read_kBs = rp.endPerformance;

	// Write speed
	cdb.data()[1] |= (int)PerformanceDataType::WriteSpeed;
	m_io->GenericPacketCall(cdb.view(), {(uint8_t*)buf.data(), (ptrdiff_t)buf.size()});
	auto wp = parse<PerformanceDescriptor>(&buf[8]);
	ret.write_kBs = wp.endPerformance;
#if 0
	auto pdl = parse<uint32_t>(buf[0]);
	std::cout << "Performance data len: " << pdl << "\n";
	bool write = bit<1>(buf[4]);
	bool except = bit<0>(buf[4]);
	std::cout << boost::format("write %i, except %i\n") % write % except;

	int nrFields = (pdl-4) / performanceDescriptorSize;
	if(nrFields > 0)
	{
		auto slba = parse<uint32_t>(buf[8]);
		auto sp = parse<uint32_t>(buf[12]);
		auto elba = parse<uint32_t>(buf[16]);
		auto ep = parse<uint32_t>(buf[20]);
		std::cout << boost::format("lba %i .. %i: performance %i .. %i\n") % slba % elba % sp % ep;
	}
#endif
	return ret;
}


int GenericPacket::UnitReady()
{
	CDB cdb;
	cdb.SetCommand(GenericPacketCommand::TestUnitReady);

	buffer_view<uint8_t> empty(nullptr, 0);
	int r = m_io->GenericPacketCall(cdb.view(), empty);

	return r;
}


void GenericPacket::GetConfiguration()
{
	std::array<uint8_t, 1 << 16> buf;

	RequestedType rt = OnlyCurrent;

	CDB cdb;
	cdb.SetCommand(GenericPacketCommand::GetConfiguration);
	SetBits<0, 2>(cdb.data()[1], rt);
	cdb.SetReadLength16(static_cast<uint16_t>(buf.size()));

	m_io->GenericPacketCall(cdb.view(), {(uint8_t*)buf.data(), (ptrdiff_t)buf.size()});

	FeatureHeader hdr = parse<FeatureHeader>(buf.data());

	assert(!"Not implmented");
}


GenericPacket::DiscStatus GenericPacket::GetDiscStatus()
{
	std::array<uint8_t, 34> buf{0};

	CDB cdb;
	cdb.SetCommand(GenericPacketCommand::DiscInformation);
	cdb.SetReadLength16(static_cast<uint16_t>(buf.size()));

	m_io->GenericPacketCall(cdb.view(), {(uint8_t*)buf.data(), (ptrdiff_t)buf.size()});

	::DiscInformation di = parse<::DiscInformation>(buf.data());

	GenericPacket::DiscStatus r;
	if(di.length == 0)
		r = DiscStatus::NoInfo;
	else if(di.discType == CddaOrCdrom)
		r = DiscStatus::Audio;
	else if(di.discType == XA)
		r = DiscStatus::XA1;

	return r;
}


struct WriteSpeedDescriptor
{
	enum WriteRotationControl
	{
		Default = 0,
		CAV = 1
	};
	bool mrw; ///< Speed suitable for Mixture of Read/Write
	WriteRotationControl wrc;
	uint32_t endLba;
	uint32_t readSpeed, writeSpeed;
};


template<>
WriteSpeedDescriptor parse(const void* pBuf)
{
	const char* cBuf = reinterpret_cast<const char*>(pBuf);
	// Table 196
	WriteSpeedDescriptor r;
	r.mrw = GetBit<0>(cBuf[0]);
	r.wrc = static_cast<WriteSpeedDescriptor::WriteRotationControl>(GetBits<3, 2>(cBuf[0]));
	r.endLba = parse<uint32_t>(&cBuf[4]);
	r.readSpeed = parse<uint32_t>(&cBuf[8]);
	r.writeSpeed = parse<uint32_t>(&cBuf[12]);
	return r;
}


GenericPacket::PerfomanceInfo GenericPacket::GetMinimumWriteSpeed()
{
	// SFF8090i-v5, page 285, table 195
	const int maxNrDescriptors = 1;
	std::array<uint8_t, 8 + maxNrDescriptors * 16> buf;

	CDB cdb;
	cdb.SetCommand(GenericPacketCommand::Performance);
	cdb.data()[1] = (int)PerformanceDataType::Tolerance | (int)PerformanceDataType::ExceptNominal;
	cdb.data()[10] = (int)PerformanceType::WriteSpeed;
	cdb.data()[9] = maxNrDescriptors;

	m_io->GenericPacketCall(cdb.view(), {(uint8_t*)buf.data(), (ptrdiff_t)buf.size()});

	auto& desc = buf[8];
	auto wp = parse<WriteSpeedDescriptor>(&desc);
#if 0
	std::cout << "mrw = " << bit<0>(desc) << "\n";
	std::cout << "exact = " << bit<1>(desc) << "\n";
	std::cout << "RDD = " << bit<2>(desc) << "\n";

	std::cout << "Speed.lba = " << wp.endLba << "\n";
	std::cout << "Speed.read = " << wp.readSpeed << " B/s\n";
	std::cout << "Speed.write = " << wp.writeSpeed << " B/s\n";
#endif
	PerfomanceInfo r;
	r.read_kBs = wp.readSpeed / 1000;
	r.write_kBs = wp.writeSpeed / 1000;
	return r;
};


template<>
TocEntry parse(const void* buf)
{
	const uint8_t* p = reinterpret_cast<const uint8_t*>(buf);
	TocEntry e;
	e.adr = static_cast<TocAdr>(GetBits<4, 4>(p[1]));
	e.control = static_cast<TocControl>(GetBits<0, 4>(p[1]));
	e.track = p[2];
	e.minute = p[5];
	e.second = p[6];
	e.frame = p[7];
	e.isrc[0] = '\0';
	return e;
}


Toc GenericPacket::GetToc(bool isrc, bool cdtext)
{
	std::array<uint8_t, 12> buf;
	CDB cdb;
	cdb.SetCommand(GenericPacketCommand::ReadToc);
	cdb.SetReadLength16(static_cast<uint16_t>(buf.size()));
	cdb.data()[2] = (int)ReadToc::Toc;
	m_io->GenericPacketCall(cdb.view(), {buf.data(), (ptrdiff_t)buf.size()});

	int trk0 = buf[2];
	int trk1 = buf[3];

	Toc toc;
	toc.upc = UniversalProductCode().second;

	for(int trk = trk0; trk <= trk1 + 1; trk++)
	{
		cdb.data()[1] = 0x02; // MSF
		cdb.data()[6] = (trk > trk1) ? int(TrackId::LeadOut) : trk;
		m_io->GenericPacketCall(cdb.view(), {buf.data(), (ptrdiff_t)buf.size()});

		toc.tracks.push_back(parse<TocEntry>(&buf[4]));
		if(isrc)
			toc.tracks.back().isrc = GetISRC(trk);
	}

	if(cdtext)
	{
		auto cdt = GetCdText();
		if(cdt.titles.count(0))
			toc.title = cdt.titles.at(0);
		if(cdt.performer.count(0))
			toc.performer = cdt.performer.at(0);

		for(auto& tr : toc.tracks)
		{
			if(cdt.titles.count(tr.track))
				tr.title = cdt.titles.at(tr.track);
			if(cdt.performer.count(tr.track))
				tr.performer = cdt.performer.at(tr.track);
		}
	}

	return toc;
}


GenericPacket::CdText GenericPacket::GetCdText()
{
	const int SessionNumber = 0;

	std::vector<uint8_t> buf(9216);

	CDB cdb;
	cdb.SetCommand(GenericPacketCommand::ReadToc);
	cdb.SetReadLength16(static_cast<uint16_t>(buf.size()));
	cdb.data()[2] = (int)ReadToc::CdText;
	cdb.data()[6] = SessionNumber;

	// Read the length
	m_io->GenericPacketCall(cdb.view(), buffer_view<uint8_t>{buf});

	// Parse
	int size = std::min<int>(parse<uint16_t>(&buf[0]), (int)buf.size());
	if(size <= 2)
		return {};
	buffer_view<const char> cdText(reinterpret_cast<char*>(&buf[4]), size - 2);

	const static int PackDataSize = 18;
	int nrPackets = (int)cdText.size() / PackDataSize;

	CdTextPacketCollectorSimple titlesCollector, performerCollector;
	for(int ipack = 0; ipack < nrPackets; ipack++)
	{
		CdTextPackData cdp = parse<CdTextPackData>(&cdText[ipack * PackDataSize]);
		if(cdp.packType == CdTextPackData::Title)
			titlesCollector.Add(cdp.trackNr, cdp.characterPosition, cdp.text);
		else if(cdp.packType == CdTextPackData::Performer)
			performerCollector.Add(cdp.trackNr, cdp.characterPosition, cdp.text);
		// else
		//	std::cout << "packType " << cdp.packType << "\n";
	}

	CdText r;
	r.titles = titlesCollector.Flush();
	r.performer = performerCollector.Flush();
	return r;
}


std::pair<int, std::string> GenericPacket::UniversalProductCode()
{
	std::array<char, 28> buf;
	CDB cdb;
	cdb.SetCommand(GenericPacketCommand::ReadSubChannel);
	cdb.SetReadLength16(static_cast<uint16_t>(buf.size()));
	cdb.data()[2] = 1 << 6; // Request SubQ data
	cdb.data()[3] = (uint8_t)SubchannelTrack::MediaCatalogNumber;

	m_io->GenericPacketCall(cdb.view(), {(uint8_t*)buf.data(), (ptrdiff_t)buf.size()});

	const char* mcnData = &buf[4];
	const char* mcn = &buf[8];
	int Aframe = mcn[15];

	int subQfmt = mcnData[0];
	assert(subQfmt == 0x02);
	bool mcValid = GetBit<7>(mcn[0]);
	if(!mcValid)
		return std::make_pair(Aframe, "");

	return std::make_pair(Aframe, std::string{mcn + 1, 12});
}


std::array<char, 12> GenericPacket::GetISRC(int track)
{
	const int IsrcSize = 12;
	std::array<char, 28> buf;
	CDB cdb;
	cdb.SetCommand(GenericPacketCommand::ReadSubChannel);
	cdb.SetReadLength16(static_cast<uint16_t>(buf.size()));

	cdb.data()[2] = 1 << 6; // Request SubQ data
	cdb.data()[3] = (uint8_t)SubchannelTrack::ISRC;
	cdb.data()[6] = track;

	m_io->GenericPacketCall(cdb.view(), {(uint8_t*)buf.data(), (ptrdiff_t)buf.size()});

	// 4 bytes of header, then 4 bytes of track ISRC data block, then the ISRC block:
	const char* isrcData = &buf[8];

	bool tcValid = GetBit<7>(isrcData[0]);
	if(!tcValid)
		return {'\0'};

	int Aframe = isrcData[14];
	std::array<char, 12> iscr;
	memcpy(iscr.data(), isrcData + 1, IsrcSize);
	return iscr;
}


int GenericPacket::SizeInSectors()
{
	std::array<uint8_t, 12> buf{0};

	CDB cdb;
	cdb.SetCommand(GenericPacketCommand::ReadToc);
	cdb.data()[1] = 0; // LBA = 0, MSF = 2
	cdb.data()[1] = (int)ReadToc::Toc;
	cdb.SetStartTrack((int)TrackId::LeadOut);
	cdb.SetReadLength16(static_cast<uint16_t>(buf.size()));

	m_io->GenericPacketCall(cdb.view(), {buf.data(), (ptrdiff_t)buf.size()});

	int lsn = parse<uint32_t>(&buf[8]);
	return lsn;
}


void GenericPacket::GetStatus(std::set<EventNotificationClass> mask, bool blocking)
{
	std::array<uint8_t, 16> buf{0};

	CDB cdb;
	cdb.SetCommand(GenericPacketCommand::GetEvent);
	cdb.SetReadLength16(static_cast<uint16_t>(buf.size()));
	cdb.data()[1] = blocking ? 0 : 1;

	m_io->GenericPacketCall(cdb.view(), {buf.data(), (ptrdiff_t)buf.size()});
	auto dataLen = parse<uint16_t>(&buf[0]);
	bool noEventAvailable = GetBit<2>(buf[2]);
	int notificationClass = buf[2] & 0x7;
	EventNotificationClass eventClasses = (EventNotificationClass)buf[3];
}


enum class ReadHeaders
{
	None = 0,
	Mode1 = 1,
	SubHeader = 2,
	All = 3
};


enum class ReadSubChannelSelection
{
	None = 0,
	Raw = 1,
	Q = 2,
	RW = 4
};


enum class ReadErrorFlags
{
	None = 0,
	C2only = 1,
	C2andBlock = 2
};


struct Qdata
{
	TocControl control;
	TocAdr adr;
	uint8_t track;
	uint8_t index;
	MSF msf, msfA;
	uint16_t crc;
};


template<>
Qdata parse(const void* buf)
{
	const uint8_t* p = reinterpret_cast<const uint8_t*>(buf);
	Qdata r;
	r.adr = static_cast<TocAdr>(p[0] >> 4);
	r.control = static_cast<TocControl>(p[0] & 0xF);
	r.track = p[1];
	r.index = p[2];
	r.msf.minute = p[3];
	r.msf.second = p[4];
	r.msf.frame = p[5];
	r.msfA.minute = p[7];
	r.msfA.second = p[8];
	r.msfA.frame = p[9];
	r.crc = parse<uint16_t>(&p[10]);
	return r;
}


int GenericPacket::ReadCd(int lba, int nrBlocks, bool)
{
	const bool sync = true;							   // page 350
	const ReadErrorFlags ref = ReadErrorFlags::C2only; // my drive doesn't support this...
	const ReadSubChannelSelection sub = ReadSubChannelSelection::Q;

	CDB cdb;
	cdb.SetCommand(GenericPacketCommand::ReadCD);
	store<uint32_t>(cdb.data()[2], lba);
	cdb.SetReadLength(nrBlocks);

	uint8_t& flagBits = cdb.data()[9];
	SetBits<7, 1>(flagBits, sync);					// Sync field
	SetBits<5, 2>(flagBits, (int)ReadHeaders::All); // Header code
	SetBits<4, 1>(flagBits, 1);						// User Data: audio data
	SetBits<3, 1>(flagBits, 1);						// ECC
	SetBits<1, 2>(flagBits, (int)ref);				// Error(s)

	uint8_t& byte10 = cdb.data()[10];
	SetBits<0, 3>(byte10, (int)sub);

	CddaFrame frame = CddaFrame::Audio;
	if(ref == ReadErrorFlags::C2only)
		frame = CddaFrame::AudioError;
	if(ref == ReadErrorFlags::C2andBlock)
		frame = CddaFrame::AudioBlockError;

	int requiredBufferSize = (int)frame;

	// std::cout << "READ CD command\n"; hexdump(cdb.data().data(), cdb.data().size());
	m_buf.fill(0xAB);

	assert(requiredBufferSize <= m_buf.size());
	m_io->GenericPacketCall(cdb.view(), {m_buf.data(), (int)m_buf.size()});

	std::cout << "Read Audio buffer\n";
	hexdump(m_buf.data(), (int)m_buf.size());

	buffer_view<uint8_t> audio(&m_buf[0], (int)CddaFrame::Audio);
	buffer_view<uint8_t> C2(&m_buf[(int)CddaFrame::Audio], 294);
	uint8_t block = m_buf[(int)CddaFrame::AudioError];

	const uint8_t* pbuf = &m_buf[(int)frame];
	if(sub == ReadSubChannelSelection::Q)
	{
		Qdata q = parse<Qdata>(pbuf);
		pbuf += 16;
		std::cout << boost::format("a %i, c %i, track %2i, idx %i, ") % (int)q.adr %
						 (int)q.control % (int)q.track % (int)q.index;
		std::cout << boost::format("msf %2i:%02i:%03i, crc %04xh\n") % q.msf.minute % q.msf.second %
						 q.msf.frame % q.crc;
	}

	int c2errors = -1;
	int blockeb = 0;
	if(ref != ReadErrorFlags::None)
	{
		c2errors = 0;
		for(auto c2 : C2)
		{
			c2errors += __builtin_popcount(c2);
			blockeb |= c2;
		}
	}

	if(block != 0 || blockeb != 0)
	{
		std::cout << boost::format("block %3i, blockeb %3i, c2errors %3i\n") % (int)block %
						 blockeb % c2errors;
	}

#if 0
	std::cout << "Audio data\n";
	hexdump(&audio, 64);

	std::cout << "C2 data\n";
	hexdump(&C2, 294);
#endif
	// Qdata qdata = parse<Qdata>(m_buf[0]);
	return c2errors;
}


void GenericPacket::FlushCache()
{
	CDB cdb;
	cdb.SetCommand(GenericPacketCommand::FlushCache);
	m_io->GenericPacketCall(cdb.view(), {nullptr, 0});
}


std::ostream& operator<<(std::ostream& os, GenericPacket::DiscStatus s)
{
	using DS = GenericPacket::DiscStatus;
	const std::map<DS, std::string> lut = {
		{DS::Error, "Error"},
		{DS::NoInfo, "No Info"}, {DS::NoDisc, "No Disc"}, {DS::Audio, "Audio"},
		{DS::Data1, "Data"}, {DS::Data2, "Data"}, {DS::XA1, "XA"}
	};
	if(lut.count(s))
		os << lut.at(s);
	else
		os << "Unknown: " + std::to_string((int)s);
	return os;
}
