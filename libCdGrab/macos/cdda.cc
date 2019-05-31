/*

Get a device list:
https://developer.apple.com/library/archive/samplecode/CDROMSample/Listings/CDROMSample_CDROMSample_c.html#//apple_ref/doc/uid/DTS10000423-CDROMSample_CDROMSample_c-DontLinkElementID_3

Sending SCSI commands:

https://developer.apple.com/library/archive/documentation/DeviceDrivers/Conceptual/WorkingWithSAM/WWS_SAMDevInt/WWS_SAM_DevInt.html

https://developer.apple.com/library/archive/documentation/DeviceDrivers/Conceptual/WorkingWithSAM/WWS_SAMDevInt/WWS_SAM_DevInt.html#//apple_ref/doc/uid/TP30000387-BCIIEGJJ

and

https://github.com/jobermayr/cdrtools/blob/master/libscg/scsi-mac-iokit.c

*/

#include <cdgrab/cdda.h>

#include <iostream>
#include <string>




#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOMediaBSDClient.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOCDMedia.h>
#include <IOKit/storage/IOCDTypes.h>
#include <CoreFoundation/CoreFoundation.h>


#include "mediaIterator.h"


GRAB_EXPORT std::vector<std::string> GetDevices()
{
	std::vector<std::string> r;

	CFMutableDictionaryRef classesToMatch = IOServiceMatching(kIOCDMediaClass);
	if(classesToMatch == nullptr)
		throw std::runtime_error("Could not instantiate Cd Media service matcher");

	CFDictionarySetValue(classesToMatch, CFSTR(kIOMediaRemovableKey), kCFBooleanTrue);

	MediaIterator mit(classesToMatch);
	for(auto medium = mit.next(); medium; medium = mit.next())
	{
		std::string pt = GetBSDPath(medium);
		if(!pt.empty())
			r.push_back(pt);
	}

	return r;
}



struct CDDA::Impl
{
	Impl(std::string devname):
		dev(devname)
	{
	}

	std::string dev;
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
	return 0;
}


int CDDA::ReadAudio(int LBA, buffer_view<int16_t> dst, int nrFrames)
{
	return 0;
}


CDDA::DriveStatus CDDA::GetDriveStatus()
{
	CDDA::DriveStatus r;
	return r;
}


CDDA::DiscStatus CDDA::GetDiscStatus()
{
	CDDA::DiscStatus di = GenericPacket::GetDiscStatus();
	return di;
}


void CDDA::Eject()
{
}


std::ostream& operator<<(std::ostream& os, CDDA::DriveStatus s)
{
	return os;
}
