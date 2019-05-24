#pragma once

#include <fcntl.h>

#include <string>

#include <cdgrab/buffer_view.h>


/// RAII wrapper for an device of the operating system
class Device
{
public:
	Device();

	Device(const std::string& pathname, int flags=O_RDONLY, int mode = 0);

	Device(const Device&) = delete;

	Device(Device&&);

	~Device();

	Device& operator=(const Device&) = delete;

	bool isOpen() const;

	int Ioctl(unsigned long request);

	int Ioctl(unsigned long request, buffer_view<void> data);

private:
	int handle;
};
