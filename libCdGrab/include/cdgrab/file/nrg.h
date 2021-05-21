#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <boost/iostreams/device/mapped_file.hpp>



/// Nero Burner's image format
class Nrg
{
public:
	struct CuePoint
	{
		enum Mode: uint8_t
		{
			Audio = 0x01,
			AudioNonCopyRighted = 0x21,
			Data = 0x41
		};

		Mode mode;
		uint8_t trackNumber;
		uint8_t indexNumber;
		uint32_t lba;
	};

	struct Cue
	{
		Cue() = default;
		const char* Parse(const char* pData);

		uint32_t size;
		std::vector<CuePoint> points;
	};

	struct DaoTrack
	{
		enum DataMode
		{
			Audio = 0x700,
			AudioSub = 0x1000
		};

		std::array<char, 12> isrc;
		uint16_t sectorSize;
		DataMode dataMode;
		uint64_t index0;
		uint64_t index1;
		uint64_t indexEnd;
	};

	struct Dao
	{
		const char* Parse(const char* pData);

		enum Toc
		{
			Audio = 0x0,
			Data = 0x1,
			Mode2Data = 0x2001
		};

		uint32_t size;
		uint32_t size2;
		std::array<char, 13> upc;
		Toc toc;
		uint8_t firstTrack;
		uint8_t lastTrack;

		std::vector<DaoTrack> tracks;
	};

	struct CdTextEntry
	{
		uint8_t packType;
		uint8_t packTypeTrack;
		uint8_t packNumber;
		uint8_t blockNumber;
		std::array<char, 12> text;
		uint16_t crc;
	};

	struct CdText
	{
		const char* Parse(const char* pData);

		uint32_t size;
		std::vector<CdTextEntry> entries;
	};

	struct SessionInfo
	{
		const char* Parse(const char* pData);

		uint32_t size;
	};

	Nrg(std::string fname);

	const std::vector<DaoTrack>& Tracks();

private:
	bool ParseHeader();
	bool ParseChunk(std::uint64_t offset);

	boost::iostreams::mapped_file_source m_file;
	bool m_version5;
	std::uint64_t m_offset;
	std::vector<CuePoint> m_cues;
	Dao m_dao;
	std::vector<CdTextEntry> m_cdText;
};
