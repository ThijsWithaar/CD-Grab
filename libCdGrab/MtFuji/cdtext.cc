#include "cdtext.h"

#include <iostream>
#include <string.h>


template<>
CdTextPackData parse(const void* buf)
{
	const uint8_t* p = reinterpret_cast<const uint8_t*>(buf);
	CdTextPackData r;
	r.packType = static_cast<CdTextPackData::PackType>(p[0]);
	r.trackNr = GetBits<0,7>(p[1]);
	r.seqNr = p[2];
	r.blockNr = GetBits<4,3>(p[3]);
	r.characterPosition = GetBits<0,4>(p[3]);
	std::copy(&p[4], &p[16], r.text.begin());
	r.crc = parse<uint16_t>(&p[16]);
	return r;
}


template<size_t N>
static std::string_view sv(std::array<char, N>& a)
{
	return std::string_view(a.data(), strnlen(a.data(), N));
}


template<size_t N>
static std::string esc(std::array<char, N>& a)
{
	std::string r(a.size(), ' ');
	for(size_t n=0; n < a.size(); n++)
		r[n] = a[n] <= 0x1f ? '_' : a[n];
	return r;
}



CdTextPacketCollectorSimple::CdTextPacketCollectorSimple():
	m_track(0)
{
}


void CdTextPacketCollectorSimple::Add(int track, int pos, std::array<char, 12>& text)
{
	for(int n=0; n < text.size(); n++)
	{
		if(text[n] == '\0')
			m_track++;
		else
			perTrack[m_track] += text[n];
	}
}


const std::map<int, std::string>& CdTextPacketCollectorSimple::Flush()
{
	return perTrack;
}


//-- CdTextPacketCollector --


CdTextPacketCollector::CdTextPacketCollector():
	prevTrack(-1)
{
}


void CdTextPacketCollector::Add(int track, int pos, std::array<char, 12>& text)
{
	//std::cout << boost::format("\nTrack %2i, pos %2i: '%s'\n") % track % pos % esc(text);

	if(prevTrack >= 0)
	{
		// max. characters of current track from the previous buffer
		size_t posc = std::min<size_t>(pos, prevText.size());
		char* pSvc = prevText.data() + prevText.size() - posc;

		// previous track starts before that, but can be zero-leading!
		char* pSvp = pSvc - 2;		// Skip the leading zero of pSvc
		while(pSvp >= prevText.data() && pSvp[-1] != '\0')
			pSvp--;
		pSvp = std::max(prevText.data(), pSvp);

		assert(pSvc >= pSvp -1);
		std::string_view svp{pSvp, (size_t)std::max<ptrdiff_t>(pSvc - pSvp -1, 0)};
		std::string_view svc{pSvc, posc};

		//std::cout << boost::format("\tPrevious(%2i) '%s', current(%2i) '%s'\n") % prevTrack % svp % track % svc;

		perTrack[prevTrack] += svp;
		perTrack[track] += svc;
	}
	prevTrack = track;
	prevText = text;
}


const std::map<int, std::string>& CdTextPacketCollector::Flush()
{
	if(prevText[0] != '\0')
		perTrack[prevTrack] += sv(prevText);
	prevText[0] = '\0';
	return perTrack;
}
