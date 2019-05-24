#include <cdgrab/cdda.h>

#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

#include <assert.h>
#include <boost/format.hpp>


// For CDDA
#include <limits.h>
#include <linux/cdrom.h>

#include <scsi/sg.h>


// For MountEntries
#include <mntent.h>
#include <stdio.h>

#include "linux/mmc.h"



// https://github.com/metabrainz/libdiscid/blob/master/src/disc_linux.c
int ScsiCommand(Device& dev, buffer_view<const uint8_t> cmd, buffer_view<uint8_t> data)
{
	const int timeout_ms = 30 * 1000;
	std::array<uint8_t, SG_MAX_SENSE> sense;
	sg_io_hdr_t io_hdr;
	memset(&io_hdr, 0, sizeof io_hdr);

	io_hdr.interface_id = 'S'; // CROM-IOCTL
	io_hdr.cmdp = const_cast<uint8_t*>(cmd.data());
	io_hdr.cmd_len = cmd.size();
	io_hdr.timeout = timeout_ms;
	io_hdr.sbp = sense.data();
	io_hdr.mx_sb_len = sense.size();
	io_hdr.flags = SG_FLAG_DIRECT_IO;

	io_hdr.dxferp = (void*)data.data();
	io_hdr.dxfer_len = data.size();
	io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;

	return dev.Ioctl(SG_IO, buffer_view<void>(&io_hdr, sizeof(io_hdr)));
}


bool isCdrom(std::string deviceName, const std::string* pMountType = nullptr)
{
	if(pMountType)
	{
		std::cout << "Checking " << deviceName << ", type " << *pMountType << "\n";
		return *pMountType == "iso9660";
	}

	bool is_cd = false;
	Device cdfd(deviceName, O_RDONLY | O_NONBLOCK, 0);
	if(cdfd.isOpen())
		if(cdfd.Ioctl(CDROM_GET_CAPABILITY, buffer_view<void>(nullptr, 0)) != -1)
			is_cd = true;

	// std::cout << "Checking " << deviceName << " result " << is_cd << "\n";
	return is_cd;
}


class NameRange
{
public:
	NameRange(std::string base, int from, int to): base(base), from(from), to(to), number(true)
	{
	}

	NameRange(std::string base, char from, char to): base(base), from(from), to(to), number(false)
	{
	}

	struct Iterator
	{
		int pos;
		NameRange* parent;

		bool operator!=(const Iterator& o)
		{
			return pos != o.pos;
		}

		Iterator& operator++()
		{
			pos++;
			return *this;
		}

		std::string operator*()
		{
			if(parent->number)
				return parent->base + std::to_string(pos);
			else
				return parent->base + static_cast<char>(pos);
		}
	};

	Iterator begin()
	{
		return Iterator{from, this};
	}

	Iterator end()
	{
		return Iterator{to, this};
	}

private:
	std::string base;
	int from;
	int to;
	bool number;
};


class MountEntries
{
public:
	MountEntries(std::string mtab)
	{
		mntfp = setmntent(mtab.c_str(), "r");
		if(mntfp == nullptr)
			throw std::runtime_error("Could not open mount entries");
	}

	~MountEntries()
	{
		if(mntfp != NULL)
			endmntent(mntfp);
	}

	struct Iterator
	{
		struct mntent* mntent;
		MountEntries* parent;

		bool operator!=(const Iterator& o)
		{
			return mntent != o.mntent;
		}

		Iterator& operator++()
		{
			assert(parent && parent->mntfp);
			mntent = getmntent(parent->mntfp);
			return *this;
		}

		std::pair<std::string, std::string> operator*()
		{
			return {mntent->mnt_fsname, mntent->mnt_type};
		}
	};

	Iterator begin()
	{
		return Iterator{getmntent(mntfp), this};
	}

	Iterator end()
	{
		return Iterator{nullptr, this};
	}

private:
	FILE* mntfp;
};


std::vector<std::string> GetDevices()
{
	const std::array<std::string, 2> fixed = {"/dev/cdrom", "/dev/dvd"};
	const std::array<NameRange, 1> range = {
		NameRange{"/dev/hd", 'a', 'f'},
		//		NameRange{"/dev/sd" , 'a', 'z'},
		//		NameRange{"/dev/scd",  0,  27 },
		//		NameRange{"/dev/sr" ,  0,  27 },
	};
	const std::array<std::string, 1> mtabs = {"/etc/fstab"}; // "/etc/mtab"

	std::vector<std::string> devices;
	auto check = [&](const std::string& name, const std::string* fstype) {
		if(isCdrom(name, fstype))
			devices.push_back(name);
	};

	for(auto name : fixed)
		check(name, nullptr);
#if 0
	for(auto gen: range)
		for(auto name: gen)
			check(name, nullptr);
	for(auto tab: mtabs)
		for(auto it: MountEntries(tab))
			check(it.first, &it.second);
#endif
	return devices;
}


std::ostream& operator<<(std::ostream& os, cdrom_tocentry e)
{
	os << boost::format("track %2i, adr %i, ctrl %i, format %i\n") % e.cdte_track %
			  int(e.cdte_adr) % int(e.cdte_ctrl) % e.cdte_format;
	return os;
}


//-- CDDA --


struct CDDA::Impl
{
	Impl(Device&& device, CommunicationChannel channel):
		dev(std::move(device)),
		channel(channel)
	{
	}

	Device dev;
	CommunicationChannel channel;
	MMC mmc;
};


CDDA::CDDA(std::string devicename, CommunicationChannel channel):
	GenericPacket(this),
	m(std::make_unique<CDDA::Impl>(Device(devicename), channel))
{
}


CDDA::~CDDA()
{
}


void CDDA::GenericPacketCall(buffer_view<const uint8_t> cmd, buffer_view<uint8_t> data)
{
	int ret;
	if(m->channel == CommunicationChannel::SCSI)
	{
		ret = ScsiCommand(m->dev, cmd, data);
	}
	else
	{
		ret = m->mmc.read(m->dev, cmd, data);
	}
	if(ret != 0)
		throw std::runtime_error("GenericPacketCall error " + std::to_string(ret));
}


TocEntry CreateEntry(const cdrom_tocentry& entry)
{
	TocEntry e;
	e.track = entry.cdte_track;
	e.frame = entry.cdte_addr.msf.frame;
	e.minute = entry.cdte_addr.msf.minute;
	e.second = entry.cdte_addr.msf.second;
	e.control = static_cast<TocControl>(entry.cdte_ctrl);
	return e;
}


CDDA::DriveStatus CDDA::GetDriveStatus()
{
	int result = m->dev.Ioctl(CDROM_DRIVE_STATUS);
	return static_cast<CDDA::DriveStatus>(result);
}


CDDA::DiscStatus CDDA::GetDiscStatus()
{
	int result = m->dev.Ioctl(CDROM_DISC_STATUS);
	return static_cast<CDDA::DiscStatus>(result);
}


bool CDDA::HasMediaChanged()
{
	return m->dev.Ioctl(CDROM_MEDIA_CHANGED);
}


#if 0
Toc CDDA::GetTOCHL()
{
	cdrom_tochdr tochdr;
	m->dev.Ioctl(CDROMREADTOCHDR, buffer_view<void>(&tochdr, sizeof(tochdr)));

	cdrom_tocentry entry;
	Toc r;
	for(int trk = tochdr.cdth_trk0; trk <= tochdr.cdth_trk1; trk++)
	{
		entry.cdte_track = trk;
		entry.cdte_format = CDROM_MSF;
		m->dev.Ioctl(CDROMREADTOCENTRY, buffer_view<void>(&entry, sizeof(entry)));
		r.tracks.push_back(CreateEntry(entry));
	}

	entry.cdte_track = CDROM_LEADOUT;
	entry.cdte_format = CDROM_MSF;
	r.tracks.push_back(CreateEntry(entry));

	return r;
}



int CDDA::SizeInSectorsHl()
{
	cdrom_tocentry tocent;
	tocent.cdte_track = CDROM_LEADOUT;
	tocent.cdte_format = CDROM_LBA;
	m->dev.Ioctl(CDROMREADTOCENTRY, buffer_view<void>(&tocent, sizeof(tocent)));
	return tocent.cdte_addr.lba;
}
#endif


void CDDA::ReadSector(MSF pos, std::array<char, (int)CddaFrame::Audio>& dst)
{
	cdrom_msf& msf = *reinterpret_cast<cdrom_msf*>(dst.data());
	msf.cdmsf_min0 = pos.minute;
	msf.cdmsf_sec0 = pos.second;
	msf.cdmsf_frame0 = pos.frame;

	m->dev.Ioctl(CDROMREADRAW, buffer_view<void>{dst.data(), (int)dst.size()});
}


int CDDA::ReadAudio(int LBA, buffer_view<int16_t> dst, int nrFrames)
{
	if(dst.size()*sizeof(*dst.data()) < nrFrames * (int)CddaFrame::Audio)
		throw std::runtime_error("CDDA: Destination buffer too small");

	cdrom_read_audio ra;
	ra.addr.lba = LBA;
	ra.addr_format = CDROM_LBA;
	ra.buf = reinterpret_cast<uint8_t*>(dst.data());
	ra.nframes = nrFrames;
	return m->dev.Ioctl(CDROMREADAUDIO, buffer_view<void>{&ra, sizeof(ra)});
}


#if 0
std::string CDDA::UniversalProductCodeHL()
{
	cdrom_mcn mcn;
	m->dev.Ioctl(CDROM_GET_MCN, buffer_view<void>{&mcn, sizeof(mcn)});
	const char* mcns = reinterpret_cast<const char*>(mcn.medium_catalog_number);
	while(*mcns == '0')
		mcns++;
	return mcns;
}
#endif


void CDDA::Eject()
{
	m->dev.Ioctl(CDROMEJECT);
}


std::ostream& operator<<(std::ostream& os, CDDA::DriveStatus s)
{
	switch(s)
	{
		case CDDA::DriveStatus::Error:
			os << "Error"; break;
	case CDDA::DriveStatus::NoInfo:
		os << "No Info";
		break;
	case CDDA::DriveStatus::NoDisc:
		os << "No Disc";
		break;
	case CDDA::DriveStatus::TrayOpen:
		os << "Tray Open";
		break;
	case CDDA::DriveStatus::NotReady:
		os << "Not Ready";
		break;
	case CDDA::DriveStatus::DiscOk:
		os << "Disc Ok";
		break;
	default:
		os << "Unknown " + std::to_string((int)s);
	}
	return os;
}


std::ostream& operator<<(std::ostream& os, CDDA::DiscStatus s)
{
	switch(s)
	{
	case CDDA::DiscStatus::NoInfo:
		os << "No Info";
		break;
	case CDDA::DiscStatus::NoDisc:
		os << "No Disc";
		break;
	case CDDA::DiscStatus::Audio:
		os << "Audio";
		break;
	case CDDA::DiscStatus::Data1:
		os << "Data1";
		break;
	case CDDA::DiscStatus::Data2:
		os << "Data2";
		break;
	case CDDA::DiscStatus::XA1:
		os << "XA1";
		break;
	case CDDA::DiscStatus::XA2:
		os << "XA2";
		break;
	case CDDA::DiscStatus::Mixed:
		os << "Mixed";
		break;
	}
	return os;
}
