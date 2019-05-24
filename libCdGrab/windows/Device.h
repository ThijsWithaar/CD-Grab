#pragma once

#include <string>

#include <windows.h>

#include <cdgrab/buffer_view.h>
#include <cdgrab/grab_export.h>

const static int O_NONBLOCK = 0;
const static int O_RDONLY = 0;

struct GRAB_EXPORT Device
{
	Device(std::string name = "", int flags = 0);

	~Device();

	int Ioctl(unsigned long request, buffer_view<const void> input, buffer_view<void> output);

	HANDLE handle;
};
