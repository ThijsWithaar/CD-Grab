#include <cdgrab/file/nrg.h>

#include <iostream>

#ifdef _WIN32

#define bswap_16(value) ((((value) & 0xff) << 8) | ((value) >> 8))

#define bswap_32(value) \
(((uint32_t)bswap_16((uint16_t)((value) & 0xffff)) << 16) | \
(uint32_t)bswap_16((uint16_t)((value) >> 16)))

#define bswap_64(value) \
(((uint64_t)bswap_32((uint32_t)((value) & 0xffffffff)) << 32) | \
(uint64_t)bswap_32((uint32_t)((value) >> 32)))

#else

#include <byteswap.h>

#endif


template<typename T> T read(const char* p);


template<> uint8_t read(const char* p)
{
	return *p;
}


template<> uint16_t read(const char* p)
{
	uint16_t v;
	memcpy(&v, p, sizeof(v));
	return bswap_16(v);
}


template<> int32_t read(const char* p)
{
	int32_t v;
	memcpy(&v, p, sizeof(v));
	return bswap_32(v);
}


template<> uint32_t read(const char* p)
{
	uint32_t v;
	memcpy(&v, p, sizeof(v));
	return bswap_32(v);
}


template<> uint64_t read(const char* p)
{
	uint64_t v;
	memcpy(&v, p, sizeof(v));
	return bswap_64(v);
}



const char* Nrg::Cue::Parse(const char* pChunk)
{
	assert("CUEX" == std::string_view(pChunk, 4));
	size = read<uint32_t>(pChunk+4);
	for(const char* pCue = pChunk+8; pCue < pChunk + size; pCue +=8)
	{
		Nrg::CuePoint p;
		p.mode = static_cast<Nrg::CuePoint::Mode>(read<uint8_t>(pCue+0));
		p.trackNumber = read<uint8_t>(pCue+1);
		p.indexNumber = read<uint8_t>(pCue+2);
		p.lba = read<int32_t>(pCue+4);
		points.push_back(p);
	}
	return pChunk + 8 + size;
}


const char* Nrg::Dao::Parse(const char* pChunk)
{
	assert("DAOX" == std::string_view(pChunk, 4));
	size = read<uint32_t>(pChunk+4);
	size2 = read<uint32_t>(pChunk+8);

	memcpy(upc.data(), pChunk+12, 13);
	toc = (Toc)read<uint16_t>(pChunk + 26);
	firstTrack = read<uint8_t>(pChunk + 28);
	lastTrack = read<uint8_t>(pChunk + 29);
	for(auto pTrack = pChunk+30; pTrack < pChunk + size; pTrack+=42)
	{
		DaoTrack t;
		memcpy(t.isrc.data(), pTrack, t.isrc.size());
		t.sectorSize = read<uint16_t>(pTrack + 12);
		t.dataMode = (DaoTrack::DataMode)read<uint16_t>(pTrack + 14);

		t.index0 = read<uint64_t>(pTrack + 18);
		t.index1 = read<uint64_t>(pTrack + 26);
		t.indexEnd = read<uint64_t>(pTrack + 34);

		tracks.push_back(t);
	}

	return pChunk + 8 + size;
}


const char* Nrg::CdText::Parse(const char* pChunk)
{
	assert("CDTX" == std::string_view(pChunk, 4));
	size = read<uint32_t>(pChunk+4);

	for(auto pText = pChunk+8; pText < pChunk + size; pText+=18)
	{
		Nrg::CdTextEntry e;
		e.packType = read<uint8_t>(pText);
		e.packTypeTrack = read<uint8_t>(pText + 1);
		e.packNumber = read<uint8_t>(pText + 2);
		e.blockNumber = read<uint8_t>(pText + 3);
		memcpy(e.text.data(), pText+4, e.text.size());
		e.crc = read<uint16_t>(pText+16);

		entries.push_back(e);
	}

	return pChunk + 8 + size;
}


const char* Nrg::SessionInfo::Parse(const char* pChunk)
{
	assert("SINF" == std::string_view(pChunk, 4));
	size = read<uint32_t>(pChunk+4);


	return pChunk + 8 + size;
}


std::ostream& operator<<(std::ostream& os, Nrg::Cue c)
{
	os << "size " << c.size << "\n";
	for(auto p: c.points)
	{
		os << "mode " << (int)p.mode << ", ";
		os << "track " << (int)p.trackNumber << ", ";
		os << "index " << (int)p.indexNumber << ", ";
		os << "lba " << (int)p.lba << "\n";
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, Nrg::CdText c)
{
	return os;
}


std::ostream& operator<<(std::ostream& os, Nrg::Dao d)
{
	os << "upc         : " << std::string_view(d.upc.data(), d.upc.size()) << "\n";
	os << "first track : " << (int)d.firstTrack << "\n";
	os << "last track  : " << std::dec << (int)d.lastTrack << "\n";
	for(auto t: d.tracks)
	{
		os << "  isrc  " << std::string_view(t.isrc.data(), t.isrc.size()) << ", ";
		os << "sector size  " << std::dec << t.sectorSize << ", ";
		os << "data mode " << std::hex << (int)t.dataMode << ", ";
		os << "index0 " << t.index0 << ", ";
		os << "index1 " << t.index1 << ", ";
		os << "indexEnd " << t.indexEnd << "\n";
	}
	return os;
}


std::ostream& operator<<(std::ostream& os, Nrg::SessionInfo d)
{
	return os;
}



Nrg::Nrg(std::string fname):
	m_file(fname)
{
	ParseHeader();
}


const std::vector<Nrg::DaoTrack>& Nrg::Tracks()
{
	return m_dao.tracks;
}


bool Nrg::ParseHeader()
{
	auto pEnd = m_file.data() + m_file.size();
	auto nero = std::string_view(pEnd - 8, 4);
	auto ner5 = std::string_view(pEnd - 12, 4);

	if(nero == "NERO")
	{
		m_version5 = false;
		m_offset = read<uint32_t>(pEnd-4);
	}
	else if(ner5 == "NER5")
	{
		m_version5 = true;
		m_offset = read<uint64_t>(pEnd-8);
	}
	else
		return false;

	//std::cout << "Offset = " << std::hex << m_offset << "\n";
	ParseChunk(m_offset);

	return true;
}


bool Nrg::ParseChunk(std::uint64_t offset)
{
	auto pChunk = m_file.data() + m_offset;

	while(pChunk < m_file.end() - 8)
	{
		auto id = std::string_view(pChunk, 4);
		std::cout << "Chunk ID : " << id << "\n";

		if(id == "CUEX")
		{
			Cue cue;
			pChunk = cue.Parse(pChunk);
			m_cues = cue.points;
			//std::cout << cue << "\n";
		}
		else if(id == "DAOX")
		{
			pChunk = m_dao.Parse(pChunk);
			//std::cout << m_dao << "\n";
		}
		else if(id == "CDTX")
		{
			CdText cdText;
			pChunk = cdText.Parse(pChunk);
			m_cdText = cdText.entries;
			//std::cout << m_cdText << "\n";
		}
		else if(id == "SINF")
		{
			SessionInfo sinfo;
			pChunk = sinfo.Parse(pChunk);
			//std::cout << sinfo << "\n";
		}
		else if(id == "END!")
		{
			break;
		}
		else
		{
			std::uint32_t size = read<uint32_t>(pChunk+4);
			pChunk += 8 + size;
		}
	}

	return true;
}


