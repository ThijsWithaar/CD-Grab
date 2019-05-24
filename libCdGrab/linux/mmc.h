#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <linux/cdrom.h>


#include <cdgrab/buffer_view.h>

#include "Device.h"




class MMC
{
public:
	typedef buffer_view<uint8_t> buf;
	typedef buffer_view<const uint8_t> cbuf;

	MMC();

	int read(Device& d, cbuf cmd, buf data, int timeout_ms = -1);

	int write(Device& d, cbuf cmd, cbuf data, int timeout_ms = -1);

	int call(Device& d, cbuf cmd, int timeout_ms = -1);

private:
	int run(Device& d, cbuf cmd, buf data, int dir, int timeout_ms);

	cdrom_generic_command m_cgc;
	int m_timeout_ms;
};
