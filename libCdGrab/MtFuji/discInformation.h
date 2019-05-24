#pragma once

#include "parse.h"


/// Table 290
enum DiscStatus
{
	EmptyDisc = 0,
	IncompleteDisc = 1,
	CompleteDisc = 2,
	Others = 3 /// Non-write protected Random Writable Media
};

enum DiscType
{
	CddaOrCdrom = 0x00,
	CDi = 0x10,
	XA = 0x20,
	Undefined = 0xFF
};

struct DiscInformation
{
	uint16_t length;
	bool erasable;
	int lastSessionStatus;
	DiscStatus discStatus;
	DiscType discType;
	uint8_t nrSesions;
	uint8_t nrOpcEntries;
};

template<>
DiscInformation parse(const void* buf);
