#include "mmc.h"

#include <iostream>
#include <string.h>


MMC::MMC():
	m_timeout_ms(30*1000)
{
	memset(&m_cgc, 0, sizeof(m_cgc));
}


int MMC::read(Device& d, cbuf cmd, buf data, int timeout_ms)
{
	return run(d, cmd, data, CGC_DATA_READ, timeout_ms);
}


int MMC::write(Device& d, cbuf cmd, cbuf data, int timeout_ms)
{
	buf vdata{const_cast<uint8_t*>(data.data()), data.size()};
	return run(d, cmd, vdata, CGC_DATA_WRITE, timeout_ms);
}


int MMC::call(Device& d, cbuf cmd, int timeout_ms)
{
	buf data{nullptr, 0};
	return run(d, cmd, data, CGC_DATA_NONE, timeout_ms);
}


int MMC::run(Device& d, cbuf cmd, buf data, int dir, int timeout_ms)
{
	request_sense sense;

	memcpy(m_cgc.cmd, cmd.data(), cmd.size());
	m_cgc.buffer = data.data();
	m_cgc.buflen = data.size();
	m_cgc.sense = reinterpret_cast<request_sense*>(&sense);
	m_cgc.data_direction = dir;
	m_cgc.timeout = timeout_ms >= 0 ? timeout_ms : m_timeout_ms;

	auto r = d.Ioctl(CDROM_SEND_PACKET, buffer_view<void>(&m_cgc, sizeof(m_cgc)));
	if(r != 0)
		std::cout << "MMC::run returns " << r << "\n";
	return r;
}
