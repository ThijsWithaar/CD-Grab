#include "discInformation.h"


template<>
DiscInformation parse(const void* buf)
{
	const char* p = reinterpret_cast<const char*>(buf);
	DiscInformation r;
	r.length = parse<uint16_t>(p);
	r.erasable = GetBit<4>(p[2]);
	r.lastSessionStatus = GetBits<2, 2>(p[2]);
	r.discStatus = (DiscStatus)GetBits<0, 2>(p[2]);
	r.discType = (DiscType)p[8];
	r.nrSesions = p[9];
	r.nrOpcEntries = p[33];
	return r;
}
