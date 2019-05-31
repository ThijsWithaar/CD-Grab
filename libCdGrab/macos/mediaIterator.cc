#include "mediaIterator.h"

#include <stdexcept>

#include <sys/param.h>
#include <paths.h>

#include <IOKit/IOBSD.h>



MediaIterator::MediaIterator():
	it(IO_OBJECT_NULL)
{
}


MediaIterator::MediaIterator(CFMutableDictionaryRef& match):
	MediaIterator()
{
	kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault, match, &it);
	if(kr != KERN_SUCCESS)
		throw std::runtime_error("Could not get a matching service: " + std::to_string(kr));
}


MediaIterator::~MediaIterator()
{
	if (it != IO_OBJECT_NULL)
		IOObjectRelease(it);
}


io_object_t MediaIterator::next()
{
	return IOIteratorNext(it);
}



std::string GetBSDPath(io_object_t& medium)
{
	CFTypeRef pathProperty = IORegistryEntryCreateCFProperty(medium,
                                                            CFSTR(kIOBSDNameKey),
                                                            kCFAllocatorDefault,
                                                            0);

	CFStringRef pathString = reinterpret_cast<CFStringRef>(pathProperty);

	std::string ps;
	if(!pathString)
		return ps;

	char bsdPath[MAXPATHLEN];
	if (CFStringGetCString(pathString, bsdPath, sizeof(bsdPath), kCFStringEncodingUTF8))
	{
		// bsd defines devices with 'r' as volumes...
		ps = std::string(_PATH_DEV) + 'r' + bsdPath;
		CFRelease(pathString);
	}

	return ps;
}
