#include <cdgrab/cdda.h>

#include <Windows.h>

#include <cassert>
#include <iostream>

#include <Ntddcdrm.h>
#include <Ntddscsi.h>


#include "Device.h"


#define SENSE_BUFFER_LENGTH 32


struct SCSI_PASS_THROUGH_DIRECT_WITH_SENSE
{
	SCSI_PASS_THROUGH_DIRECT sptd;		   // to driver, contains CDB
	ULONG Filler;						   // realign buffer to double word boundary
	UCHAR ucSenseBuf[SENSE_BUFFER_LENGTH]; // buf for returned sense data
};


template<size_t N>
std::string to_string(std::array<CHAR, N> arr)
{
	return std::string{arr.begin(), arr.begin() + strnlen(arr.data(), arr.size())};
}


bool isCdrom(std::string deviceName)
{
	UINT type = GetDriveTypeA(deviceName.c_str());
	return type == DRIVE_CDROM;
}


std::string GetVolumePath(std::string& deviceName)
{
	// https : // docs.microsoft.com/en-us/windows/desktop/FileIO/displaying-volume-paths
	BOOL Success = FALSE;
	std::array<CHAR, MAX_PATH + 1> names;
	DWORD CharCount = (DWORD)names.size();
	Success =
		GetVolumePathNamesForVolumeNameA(deviceName.c_str(), names.data(), CharCount, &CharCount);
	std::cout << "Volume Name: '" << deviceName << "'\n";
	std::cout << "Volume Path: '" << names.data() << "'\n";

	// auto name = to_string(names);
	auto name = deviceName;
	name[name.size() - 1] = '\0'; // Strip backslash

	// name = "\\\\.\\" + name;	  // from path to volume
	std::cout << "Modified Path: '" << name << "'\n";

	return name;
}


std::vector<std::string> GetDevices()
{
	// https://docs.microsoft.com/en-us/windows/desktop/FileIO/displaying-volume-paths

	std::vector<std::string> r;

	std::array<CHAR, MAX_PATH + 1> volumeName;
	HANDLE FindHandle = FindFirstVolumeA(volumeName.data(), (DWORD)volumeName.size());
	if(FindHandle == INVALID_HANDLE_VALUE)
		return r;

	BOOL succes = TRUE;
	while(succes == TRUE)
	{
		std::string volStr = to_string(volumeName);
		if(isCdrom(volStr))
			r.push_back(GetVolumePath(volStr));
		succes = FindNextVolumeA(FindHandle, volumeName.data(), (DWORD)volumeName.size());
	}

	FindVolumeClose(FindHandle);
	return r;
}


//-- CDDA --


struct CDDA::Impl
{
	Impl(std::string devname): dev(devname)
	{
	}

	Device dev;
};


CDDA::CDDA(std::string deviceName, CommunicationChannel)
	: GenericPacket(this), m(std::make_unique<Impl>(deviceName))
{
}


CDDA::~CDDA()
{
}


int CDDA::GenericPacketCall(buffer_view<const uint8_t> cmd, buffer_view<uint8_t> data)
{
	SCSI_PASS_THROUGH_DIRECT_WITH_SENSE spdb;
	ZeroMemory(&spdb, sizeof(spdb));

	SCSI_PASS_THROUGH_DIRECT& spd = spdb.sptd;

	spd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	spd.PathId = 0;
	spd.TargetId = 1;
	spd.Lun = 0;
	memcpy(spd.Cdb, cmd.data(), std::min<size_t>(cmd.size(), 16));
	spd.CdbLength = static_cast<UCHAR>(cmd.size());
	spd.DataIn = SCSI_IOCTL_DATA_IN;
	spd.DataBuffer = data.data();
	spd.DataTransferLength = static_cast<ULONG>(data.size());
	spd.TimeOutValue = 5 * 1000; // in ms?
	// This is optional
	spd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_SENSE, ucSenseBuf);
	spd.SenseInfoLength = 0; // SENSE_BUFFER_LENGTH;

	auto status = m->dev.Ioctl(IOCTL_SCSI_PASS_THROUGH_DIRECT,
							   buffer_view<const void>(&spd, sizeof(spd)),
							   buffer_view<void>(&spd, sizeof(spd)));
	if(status != 1)
		throw std::runtime_error("GenericPacketCall error");

	return spd.ScsiStatus;
}


int CDDA::ReadAudio(int LBA, buffer_view<int16_t> dst, int nrFrames)
{
	const int CDROM_SECTOR_SIZE = 2048; // Yes, even though CDDA sectors are larger
	RAW_READ_INFO rawReadInfo;
	memset(&rawReadInfo, 0, sizeof(rawReadInfo));
	rawReadInfo.DiskOffset.QuadPart = (long long)LBA * CDROM_SECTOR_SIZE;
	rawReadInfo.SectorCount = nrFrames;

	rawReadInfo.TrackMode = ::CDDA;
	assert(dst.size() * sizeof(*dst.data()) >= nrFrames * (int)CddaFrame::Audio);
	// rawReadInfo.TrackMode = RawWithC2AndSubCode;
	// assert(dst.size() >= nrFrames*CD_RAW_SECTOR_WITH_C2_AND_SUBCODE_SIZE);

	int rc =
		m->dev.Ioctl(IOCTL_CDROM_RAW_READ,
					 buffer_view<const void>{&rawReadInfo, sizeof(rawReadInfo)},
					 buffer_view<void>{dst.data(), dst.size() * (ptrdiff_t)sizeof(*dst.data())});
	return rc;
}


CDDA::DriveStatus CDDA::GetDriveStatus()
{
	int rc = UnitReady();
	DriveStatus r = DriveStatus::NotReady;
	if(rc == 0x00)
		r = DriveStatus::DiscOk;
	if(rc == 0x02)
		r = DriveStatus::NoDisc;

	return r;
}


CDDA::DiscStatus CDDA::GetDiscStatus()
{
#if 1
	CDDA::DiscStatus di = GenericPacket::GetDiscStatus();
	return di;
#else
	DISK_GEOMETRY geom;
	int rc = m->dev.Ioctl(IOCTL_CDROM_GET_DRIVE_GEOMETRY,
						  buffer_view<const void>{nullptr, 0},
						  buffer_view<void>(&geom, sizeof(geom)));

	std::array<char, 64> cfgIn;
	std::array<char, 256> cfgOut;
	int rc2 = m->dev.Ioctl(IOCTL_CDROM_GET_CONFIGURATION,
						   buffer_view<const void>{cfgIn.data(), cfgIn.size()},
						   buffer_view<void>(cfgOut.data(), cfgOut.size()));
	PGET_CONFIGURATION_HEADER cfgHeader = cfgOut.data();

	DiscStatus r = DiscStatus::NoInfo;
	geom.MediaType;
	switch(geom.BytesPerSector)
	{
	case(int)CddaFrame::Audio:
		r = DiscStatus::Audio;
	default:
		r = DiscStatus::NoInfo;
	}
	return r;
#endif
}


void CDDA::Eject()
{
	m->dev.Ioctl(IOCTL_STORAGE_EJECT_MEDIA,
				 buffer_view<const void>{nullptr, 0},
				 buffer_view<void>{nullptr, 0});
}


std::ostream& operator<<(std::ostream& os, CDDA::DriveStatus s)
{
	using DS = CDDA::DriveStatus;
	switch(s)
	{
	case DS::Error:
		os << "Error";
		break;
	case DS::NoInfo:
		os << "No Info";
		break;
	case DS::NoDisc:
		os << "No Disc";
		break;
	case DS::TrayOpen:
		os << "Tray Open";
		break;
	case DS::DiscOk:
		os << "Disc OK";
		break;
	default:
		os << "Other: " + std::to_string((int)s);
		break;
	}
	return os;
}
