#pragma once

#include <array>

#include <cdgrab/gpc.h>

#include "parse.h"


// Table 496
struct CdTextPackData
{
	// Table 497
	enum PackType
	{
		Title = 0x80,
		Performer = 0x81,
		SongWriter = 0x82,
		Composer = 0x83,
		Arranger = 0x84,
		Message = 0x85,
		DiscIdInfo = 0x86,
		Genre = 0x87,
		TocInfo = 0x88,
		TocInfo2 = 0x89,
		UpcEan_ISRC = 0x8E,
		Size = 0x8F
	};

	PackType packType;
	int trackNr;
	int seqNr;
	int blockNr;
	int characterPosition;
	std::array<char, 12> text;
	uint16_t crc;
};

template<> CdTextPackData parse(const void* buf);

/// Simple collector of per-track info (artist, title).
/// Assumes tracks start at zero, and increase once per zero-terminator.
/// This is not standard-compliant, but stringlenghts < 4 are somewhat undedefined in the standard: consecutive packets can have track number differences > 1.
class CdTextPacketCollectorSimple
{
public:
	CdTextPacketCollectorSimple();

	void Add(int track, int pos, std::array<char, 12>& text);

	const std::map<int, std::string>& Flush();
private:
	std::map<int, std::string> perTrack;
	int m_track;
};

/// Tries to be standards compliant, fails if track difference between packets > 1.
class CdTextPacketCollector
{
public:
	CdTextPacketCollector();

	void Add(int track, int pos, std::array<char, 12>& text);

	const std::map<int, std::string>& Flush();
private:
	int prevTrack;
	std::array<char, 12> prevText;
	std::map<int, std::string> perTrack;
};
