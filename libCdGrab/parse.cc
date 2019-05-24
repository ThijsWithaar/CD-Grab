#include "parse.h"

#include <cstring>
#include <iostream>

#ifndef _WIN32
	#include <arpa/inet.h>
#else
	#include <windows.h>
#endif

#include <boost/format.hpp>


template<>
uint16_t parse(const void* buf)
{
	uint16_t r;
	memcpy(&r, reinterpret_cast<const uint16_t*>(buf), sizeof(r));
	return ntohs(r);
}


template<>
uint32_t parse(const void* buf)
{
	uint32_t r;
	memcpy(&r, reinterpret_cast<const uint32_t*>(buf), sizeof(r));
	return ntohl(r);
}


template<>
void store(uint8_t& buf, uint32_t data)
{
	uint32_t ndata = htonl(data);
	memcpy(&buf, &ndata, sizeof(ndata));
}


void hexdump(const uint8_t* buf, int N)
{
	int colsize = 32;

	std::cout << "BB | ";
	for(int n = 0; n < colsize; n++)
	{
		std::cout << boost::format("%02X ") % n;
		if((n & 7) == 7)
			std::cout << " ";
	}
	std::cout << std::endl;
	for(int n = 0; n < colsize * 3 + 7; n++)
		std::cout << "=";
	std::cout << std::endl;

	for(int n = 0; n < N; n++)
	{
		int row = n / colsize;
		int col = n % colsize;
		if(col == 0)
			std::cout << boost::format("%02X | ") % row;
		std::cout << boost::format("%02X ") % (int)buf[n];
		if((col & 7) == 7)
			std::cout << " ";
		if(col == colsize - 1)
			std::cout << std::endl;
	}
	std::cout << std::endl;
}
