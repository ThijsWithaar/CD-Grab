#include "Device.h"


Device::Device(std::string name, int flags)
{
	handle = CreateFile(name.c_str(),						// drive to open
		GENERIC_READ | GENERIC_WRITE,   	// read and write access to the drive
		FILE_SHARE_READ | FILE_SHARE_WRITE, // share mode
		NULL,								// default security attributes
		OPEN_EXISTING,                       // disposition
		0,                                   // file attributes
		NULL);                               // do not copy file attributes
}


Device::~Device()
{
}


int Device::Ioctl(unsigned long request, buffer_view<const void> input, buffer_view<void> output)
{
	DWORD dwJunk = 0;
	int iReply = DeviceIoControl(handle, request,
		const_cast<void*>(input.data()), (DWORD)input.size(),
		output.data(), (DWORD)output.size(),
		&dwJunk, (LPOVERLAPPED)NULL);
	return iReply;
}
