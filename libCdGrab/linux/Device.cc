#include "Device.h"

#include <iostream>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>	// close()
#include <sys/ioctl.h>


Device::Device():
	handle(-1)
{
}


Device::Device(const std::string& pathname, int flags, int mode)
{
	handle = open(pathname.c_str(), flags, mode);
}


Device::Device(Device&& o):
	handle(-2)
{
	std::swap(handle, o.handle);
}


Device::~Device()
{
	if(handle >= 0)
		close(handle);
}


bool Device::isOpen() const
{
	return handle >= 0;
}


int Device::Ioctl(unsigned long request)
{
	return ioctl(handle, request);
}


int Device::Ioctl(unsigned long request, buffer_view<void> data)
{
	return ioctl(handle, request, data.data());
}
