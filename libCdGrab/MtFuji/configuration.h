/// Section 13.4: Get Configuration
#pragma once

#include "parse.h"


/// Table 111
enum RequestedType
{
	AllSupported = 0,
	OnlyCurrent = 1,
	OnlyOne = 2
};

/// Table 113
struct FeatureHeader
{
	uint32_t length;
	uint16_t profile;
};

template<>
FeatureHeader parse(const void* buf);

/// Table 114
enum Features {
		ProfileList = 0x0,
		Core = 0x1,
		Morphing = 0x2,
};

/// Table 118
enum Profiles
{
	NonRemovableDisk = 0x1,
	CdRom = 0x8
};
