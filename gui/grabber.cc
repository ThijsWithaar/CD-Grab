#include "grabber.h"

#include <cmath>
#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>


#include <cdgrab/file/cue.h>
#include <cdgrab/file/flac.h>
#include <cdgrab/file/opus.h>

#include <cdgrab/DiscInfoDatabase.h>


std::ostream& operator<<(std::ostream& os, std::map<int, std::string> map)
{
	for(auto kv: map)
	{
		os << boost::format("%2i: '%s'\n") % kv.first % kv.second;
		os << std::flush;
	}
	return os;
}


static std::string Join(std::string sep, std::vector<std::string> list)
{
	std::string r;
	auto it = list.begin();
	if(it == list.end())
		return r;
	r = *it;
	for(++it; it != list.end(); ++it)
		r += sep + *it;
	return r;
}


static std::vector<std::string> NonEmpty(std::vector<std::string> in)
{
	std::vector<std::string> r;
	for(auto& el: in)
		if(!el.empty())
			r.push_back(el);
	return r;
}


template<size_t N>
std::string_view sv(std::array<char, N>& a)
{
	return std::string_view(a.data(), strnlen(a.data(), N));
}


template<class Archive>
void serialize(Archive & ar, Toc& t, unsigned int version)
{
	CheckSerializationVersion<Toc>(version);
	ar & BOOST_SERIALIZATION_NVP(t.upc) & BOOST_SERIALIZATION_NVP(t.tracks);
	ar & BOOST_SERIALIZATION_NVP(t.performer) & BOOST_SERIALIZATION_NVP(t.title);
}


template<class Archive>
void serialize(Archive & ar, TocEntry& e, unsigned int version)
{
	CheckSerializationVersion<TocEntry>(version);
	int control = (int)e.control;
	int adr = (int)e.adr;
	std::string isrc(sv(e.isrc));

	ar & BOOST_SERIALIZATION_NVP(e.track);
	ar & BOOST_SERIALIZATION_NVP(e.minute) & BOOST_SERIALIZATION_NVP(e.second) & BOOST_SERIALIZATION_NVP(e.frame);
	ar & BOOST_SERIALIZATION_NVP(isrc);
	ar & BOOST_SERIALIZATION_NVP(e.title) & BOOST_SERIALIZATION_NVP(e.performer);
	ar & BOOST_SERIALIZATION_NVP(control) & BOOST_SERIALIZATION_NVP(adr);

	e.isrc.fill('\0');
	memcpy(e.isrc.data(), isrc.data(), std::min(e.isrc.size(), isrc.size()));
	e.control = (TocControl)control;
	e.adr = (TocAdr)adr;
}


template<class Archive>
void serialize(Archive & ar, DiscInfo& e, unsigned int version)
{
	CheckSerializationVersion<DiscInfo>(version);
	ar & BOOST_SERIALIZATION_NVP(e.artist) & BOOST_SERIALIZATION_NVP(e.title) & BOOST_SERIALIZATION_NVP(e.year);
	ar & BOOST_SERIALIZATION_NVP(e.track) & BOOST_SERIALIZATION_NVP(e.leadout);
}


template<class Archive>
void serialize(Archive & ar, MSF& m, unsigned int version)
{
	ar & BOOST_SERIALIZATION_NVP(m.minute);
	ar & BOOST_SERIALIZATION_NVP(m.second);
	ar & BOOST_SERIALIZATION_NVP(m.frame);
}


template<class Archive>
void serialize(Archive & ar, TrackInfo& e, unsigned int version)
{
	CheckSerializationVersion<TrackInfo>(version);

	std::string isrc(sv(e.isrc));

	ar & BOOST_SERIALIZATION_NVP(e.artist) & BOOST_SERIALIZATION_NVP(e.songName) & BOOST_SERIALIZATION_NVP(isrc);
	ar & BOOST_SERIALIZATION_NVP(e.start);

	e.isrc.fill('\0');
	memcpy(e.isrc.data(), isrc.data(), std::min(e.isrc.size(), isrc.size()));
}


class SectorDbProgressForward: public SectorDbProgress
{
public:
	SectorDbProgressForward(std::function<bool(double)> f):
		m_pct(f)
	{
	}

	bool OnSectorProgress(int nrSectors, int lba, double pct) override
	{
		return m_pct(pct);
	}

private:
	std::function<bool(double)> m_pct;
};


std::ostream& operator<<(std::ostream& os, GrabberProgress::Action act)
{
	using Action = GrabberProgress::Action;
	switch(act)
	{
		case Action::TableOfContents: os << "Toc"; break;
		case Action::DiscId:os << "DiscId"; break;
		case Action::PrepareSectorDb: os << "Prepare SDb"; break;
		case Action::Reading: os << "Reading"; break;
		case Action::SaveSectorDb: os << "Saving SDb"; break;
		case Action::Process: os << "Processing"; break;
	}
	return os;
}


Grabber::Grabber()
{
}


DiscInfo Grabber::GetInfo(AudioReader& src, GrabberProgress* progress)
{
	// Read mimimal amount of TOC, to prevent slow seeking/scanning of discs already seen
	m_toc = src.GetToc(false, false);

	DiscInfo discInfo;
	if(!m_discDb.Has(m_toc))
	{
		if(progress)
			progress->OnProgress(GrabberProgress::Action::DiscId, 0);

		// Prefer data from CD-TEXT, fall back to CDDB
		m_toc = src.GetToc(true, true);
		if(!m_toc.performer.empty() && !m_toc.title.empty())
		{
			discInfo = CreateDiscInfo(m_toc);
			m_discDb.Store(discInfo);
		}
		else
			discInfo = m_discDb.Query(m_toc);
	}
	else
		discInfo = *m_discDb.Get(m_toc);
	return discInfo;
}


AudioReader& Grabber::Grab(AudioReader& m_cdda, GrabberProgress* progress)
{
	if(progress)
		progress->OnProgress(GrabberProgress::Action::TableOfContents, 0);

	auto discInfo = GetInfo(m_cdda, progress);

	if(!discInfo.upc.empty())
		std::cout << "UPC: '" << discInfo.upc << "'\n";

	auto nes = NonEmpty({discInfo.artist, discInfo.title, discInfo.upc});
	m_baseName = Join(" - ", nes);
	if(nes.empty())
		m_baseName = "dump";

	if(progress)
		progress->OnProgress(GrabberProgress::Action::PrepareSectorDb, 0);
	const std::string sectorFname = m_baseName + ".sdb";
	SectorDb::Statistics stat;
	if(boost::filesystem::is_regular_file(sectorFname))
	{
		m_sdb = SectorDb(sectorFname);
		stat = m_sdb.GetStatistics();
	}
	else
		m_sdb = SectorDb(m_cdda.SizeInSectors(), &m_toc);

	if(progress)
		progress->OnProgress(GrabberProgress::Action::PrepareSectorDb, 100.);

	const int minimumCommonCrcCount = 2;
	int readingIteration = stat.minMC;

	auto refineProgress = [progress, &readingIteration, minimumCommonCrcCount](double pct) -> bool
	{
		double pct_offset = readingIteration * 100 / minimumCommonCrcCount;
		double pct_this = pct / minimumCommonCrcCount;
		if(progress)
			return progress->OnProgress(GrabberProgress::Action::Reading, pct_offset + pct_this);
		return true;
	};
	SectorDbProgressForward sectorProgressFwd(refineProgress);

	while(stat.minMC < minimumCommonCrcCount)
	{
		int nRead = m_sdb.Refine(m_cdda, minimumCommonCrcCount, &sectorProgressFwd);
		//std::cout << boost::format("Refine read %i sectors\n") % nRead;
		if(nRead > 0)
		{
			if(progress)
				progress->OnProgress(GrabberProgress::Action::SaveSectorDb, 0);
			m_sdb.Save(sectorFname);
			if(progress)
				progress->OnProgress(GrabberProgress::Action::SaveSectorDb, 100);
		}
		else
			break;

		readingIteration++;
	}
	return m_sdb;
}


AudioReader& Grabber::LoadSectorDb(std::string sectorFname)
{
	m_sdb = SectorDb(sectorFname);
	m_baseName = boost::filesystem::basename(sectorFname);
	return m_sdb;
}


void Grabber::Encode(GrabberProgress* progress)
{
	if(progress)
		progress->OnProgress(GrabberProgress::Action::TableOfContents, 0);

	auto toc = m_sdb.GetToc(true, true);
	DiscInfo discInfo = GetInfo(m_sdb);

	// Header as .cue
	Cue cue;
	cue.datafilename = m_baseName + ".flac";
	cue.format = "FLAC";
	cue.upc = toc.upc;
	cue.tracks = toc.tracks;
	Write(cue, discInfo, m_baseName + ".cue");

	// Header serialized
	{
		std::ofstream f(m_baseName + ".xml");
		boost::archive::xml_oarchive ar(f);
		ar & BOOST_SERIALIZATION_NVP(toc) & BOOST_SERIALIZATION_NVP(discInfo);
	}

	int processLba = 0;
	int displayLba = -1;
	auto flacProgress = [&](FlacStreamEncoder::Progress p)
	{
		double mB = p.bytesWritten * std::pow(2., -20);
		MSF msf = ToMSF(processLba);
		if(processLba - displayLba > 128)
		{
			std::cout << boost::format("\r  %5.1f MB, %2i:%02i") % mB % msf.minute % msf.second;
			std::cout << std::flush;
			displayLba = processLba;
		}
	};

	// Bin+Flac
	int lbasize = m_sdb.SizeInSectors();
	int nrSamplesPerChannel = lbasize * (int)CddaFrame::Audio/4;
	FlacStreamEncoder encoder(cue.datafilename, nrSamplesPerChannel); //, flacProgress);

	//OpusEncoder encoder2(basename + ".opus");

	// Process data in chunks
	const int maxNrFrames = 64;
	std::array<uint8_t, maxNrFrames*(int)CddaFrame::Audio> buf;
	//buffer_view<uint8_t> bview_u8{buf.data(), buf.size()};
	buffer_view<int16_t > bview_i16(reinterpret_cast< int16_t*>(buf.data()), buf.size()/2);

	auto process = [&](int lba, int nrFrames)
	{
		assert(nrFrames <= maxNrFrames);

		if(progress)
		{
			double pct = 100. * lba /(lbasize-1);
			progress->OnProgress(GrabberProgress::Action::Process, pct);
		}

		// Read multiple sectors
		processLba = lba;
		m_sdb.ReadAudio(lba, bview_i16, nrFrames);

		// Per-sector processing
		for(int n=0; n < nrFrames; n++)
		{
			buffer_view<int16_t> frame{&bview_i16.data()[n*(int)CddaFrame::Audio/2], (int)CddaFrame::Audio/2};
		}

		// TODO: Checks on data (checksum, glitches)

		// TODO: replay-gain

		encoder.Append(bview_i16);
		//encoder2.Append(bview_u16, false);
	};

	// Loop, with run-out
	int lba;
	for(lba=0; lba < lbasize - maxNrFrames; lba += maxNrFrames)
	{
		process(lba, maxNrFrames);
	}
	assert(lba <= lbasize);
	assert(lbasize-lba < maxNrFrames);
	//std::cout << boost::format("run-out: lba %i, n %i. lbaSize = %i\n") % lba % (lbasize-lba) % lbasize;
	process(lba, lbasize-lba);	// run-out

	if(progress)
		progress->OnProgress(GrabberProgress::Action::Process, 100.);
}
