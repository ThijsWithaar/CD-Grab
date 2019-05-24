#pragma once

#include <cassert>
#include <cstdint>


template<typename T>
T parse(const void* buf);

template<>
uint16_t parse(const void* buf);

template<>
uint32_t parse(const void* buf);

template<typename T>
void store(uint8_t& buf, T data);

template<int B>
bool GetBit(int v)
{
	return ((v >> B) & 1) != 0;
}

template<int shift, int nrBits>
int GetBits(uint8_t src)
{
	assert(nrBits + shift <= 8);
	assert(nrBits > 0);
	int mask = (1 << nrBits) - 1;
	return (src >> shift) & mask;
}

template<int shift, int nrBits>
void SetBits(uint8_t& dst, int val)
{
	assert(nrBits + shift <= 8);
	assert(nrBits > 0);
	int mask = (1 << nrBits) - 1;
	dst |= (val & mask) << shift;
}

void hexdump(const uint8_t* buf, int N);
