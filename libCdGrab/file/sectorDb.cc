#include <cdgrab/file/sectorDb.h>

#include <iostream>
#include <fstream>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>

#include <boost/crc.hpp>

#include <boost/format.hpp>


namespace boost {
namespace serialization {


template<class Archive>
void serialize(Archive & ar, SectorDb::Sector& self, unsigned int version)
{
	CheckSerializationVersion<SectorDb::Sector>(version);
	ar & self.data & self.check & self.checks;
}


template<class Archive>
void serialize(Archive & ar, SectorDb::Checksums& self, unsigned int version)
{
	CheckSerializationVersion<SectorDb::Checksums>(version);
	ar & self.crc & self.blockErrorMask;
}


template<class Archive>
void serialize(Archive & ar, TocEntry& self, unsigned int version)
{
	CheckSerializationVersion<TocEntry>(version);
	ar & self.minute & self.second & self.frame;
	ar & self.track & self.adr & self.control;
	ar & self.isrc & self.title & self.performer;
}


template<class Archive>
void serialize(Archive & ar, Toc& self, unsigned int version)
{
	CheckSerializationVersion<Toc>(version);
	ar & self.upc & self.tracks & self.title & self.performer;
}


template<class Archive>
void serialize(Archive & ar, AudioReader::HardwareInfo& self, unsigned int version)
{
	CheckSerializationVersion<AudioReader::HardwareInfo>(version);
	ar & self.vendor & self.model & self.revision;
}


template<class Archive>
void serialize(Archive & ar, SectorDb::Data& self, unsigned int version)
{
	CheckSerializationVersion<SectorDb::Data>(version);
	ar & self.hw & self.toc & self.sectors;
}


} // namespace serialization
} // namespace boost


std::ostream& operator<<(std::ostream& os, SectorDb::Statistics s)
{
	os << "mc:[" << s.minMC << " .. " << s.maxMC << "], avg " << s.meanMC << "/" << s.meanNrReads;
	return os;
}


std::ostream& operator<<(std::ostream& os, SectorDb::Checksums c)
{
	os << boost::format("%08x.%i") % c.crc % (int)c.blockErrorMask;
	return os;
}


template<typename F, typename S>
std::ostream& operator<<(std::ostream& os, std::pair<F,S> p)
{
	os << "(" << p.first << ":" << p.second << ")";
	return os;
}


template<typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T> v)
{
	if(v.empty())
		return os;
	os << v[0];
	for(auto it = v.begin() +1; it != v.end(); ++it)
		os << "," << *it;
	return os;
}


template<typename T>
struct MostCommonOccurence
{
	void compute(const OccurenceList<T>& v)
	{
		if(v.occurences().empty())
		{
			count = 0;
			value = T();
			return;
		}
		auto& occ = v.occurences();
		auto it = std::max_element(occ.begin(), occ.end(), [](auto a, auto b){
			return a.first < b.first;
		});
		count = it->first;
		value = it->second;
	}

	int count;
	T value;
};

template<typename T>
struct MostCommon
{
	void compute(const std::vector<T>& v)
	{
		if(v.empty())
		{
			count = 0;
			value = T();
			return;
		}

		{
			s = std::set<T>(v.begin(), v.end());
			vs.assign(s.begin(), s.end());
		}

		counts.assign(vs.size(), 0);
		for(const T& el: v)
		{
			auto it = std::find(vs.begin(), vs.end(), el);
			ptrdiff_t idx = std::distance(vs.begin(), it);
			assert(idx < counts.size());
			counts[idx]++;
		}
		auto me = std::max_element(counts.begin(), counts.end());
		auto idx = std::distance(counts.begin(), me);
		count = counts.at(idx);
		value = vs.at(idx);
	}

	int count;
	T value;

private:
	std::set<T> s;
	std::vector<T> vs;
	std::vector<int> counts;
};



//-- SectorDb::Checksums --


SectorDb::Checksums::Checksums():
	crc(0),
	blockErrorMask(0)
{
}


bool operator<(const SectorDb::Checksums& a, const SectorDb::Checksums& b)
{
	if(a.crc != b.crc)
		return a.crc < b.crc;
	return a.blockErrorMask < b.blockErrorMask;
}


bool operator==(const SectorDb::Checksums& a, const SectorDb::Checksums& b)
{
	return a.crc == b.crc && a.blockErrorMask == b.blockErrorMask;
}


SectorDb::Statistics::Statistics():
	minMC(0), maxMC(0), meanMC(0),
	meanNrReads(0)
{
}


//-- SectorDb --


SectorDb::SectorDb(int nrSectors, Toc* pToc)
{
	m.sectors.resize(nrSectors);
	if(pToc)
		m.toc = *pToc;
}


SectorDb::SectorDb(const std::string& fname)
{
	std::ifstream f(fname, std::ios::binary);
	boost::archive::binary_iarchive ar(f);
	ar & m;
	std::cout << "SectorDB(): Statistics \t" << GetStatistics() << "\n";
}


SectorDb::~SectorDb()
{
}


/// Add audio only (no subframe)
void SectorDb::AddAudio(int lba, buffer_view<int16_t> audio)
{
	assert(audio.size()*sizeof(*audio.data()) == (int)CddaFrame::Audio);
	boost::crc_32_type crcr;
	crcr.process_block(audio.begin(), audio.end());
	Checksums check;
	check.crc = crcr.checksum();
	check.blockErrorMask = 0;		// Not available

	//std::cout << boost::format("AddAudio(%i): [0]=%i, crc %08x\n") % lba % audio[0] % check.crc;

	// Add CRC to list of seen:
	auto& sector = m.sectors.at(lba);
	sector.checks.push_back(check);

	// If this is now the most common one, copy data:
	//MostCommon<Checksums> mc;
	MostCommonOccurence<Checksums> mc;
	mc.compute(sector.checks);
	bool copyData = (mc.count <= 1) || (mc.value.crc != sector.check.crc);
	if(copyData)
	{
		sector.check = check;
		std::copy(audio.begin(), audio.end(), Audio(sector).begin());
		if(mc.count > 1)
			std::cout << boost::format("AddAudio(%6i): updating crc %04x, mcc %2i/%i\n") % lba % check.crc % mc.count % sector.checks.sum();
	}
}


bool SectorDbProgressStdOut::OnSectorProgress(int nrSectors, int lba, double pct)
{
	std::cout << boost::format("\rRefining: sector %6i/%6i (%5.1f%%)") % lba % nrSectors % pct;
	if(pct == 100.)
		std::cout << "\n";
	std::cout << std::flush;
	return true;
}


int SectorDb::Refine(AudioReader& reader, int minimumCommonCrcCount, SectorDbProgress* progress)
{
	if(m.toc.tracks.empty())
		m.toc = reader.GetToc();
	m.hw = reader.GetHardwareInfo();

	int nrSectors = reader.SizeInSectors();
	if(m.sectors.size() < nrSectors)
		m.sectors.resize(nrSectors);

	std::array<int16_t, (int)CddaFrame::Audio/2> buf;
	buffer_view<int16_t> bufview({buf.data(), (int)buf.size()});

	//MostCommon<Checksums> mc;
	MostCommonOccurence<Checksums> mc;

	bool cancelled = false;
	int nrRead = 0;
	for(int lba=0; lba < (int)m.sectors.size(); lba++)
	{
		//if(lba > 500) break;

		auto& sector = m.sectors[lba];
		mc.compute(sector.checks);
		if(mc.count >= minimumCommonCrcCount)
			continue;

		if(progress && nrRead % 256 == 0)
		{
			double pct = (lba*100.) / m.sectors.size();
			cancelled = !progress->OnSectorProgress((int)m.sectors.size(), lba, pct);
			if(cancelled)
				break;	// Cancelled
		}

		reader.ReadAudio(lba, bufview, 1);
		AddAudio(lba, bufview);
		nrRead++;
	}

	if(!cancelled && progress)
		progress->OnSectorProgress((int)m.sectors.size(), (int)m.sectors.size(), 100.);

	return nrRead;
}


/// Get sector at index lba
const SectorDb::Sector& SectorDb::operator[](int lba)
{
	return m.sectors.at(lba);
}


void SectorDb::Save(const std::string& fname)
{
	std::ofstream f(fname, std::ios::binary);
	boost::archive::binary_oarchive ar(f);
	ar & m;
	std::cout << "SectorDB.Save(): Statistics\t" << GetStatistics() << "\n";
}


SectorDb::Statistics SectorDb::GetStatistics()
{
	SectorDb::Statistics s;
	s.maxMC = 0;
	s.minMC = std::numeric_limits<int>::max();
	std::array<double, 3> mcc{0}, nrReads{0};

	//MostCommon<Checksums> mc;
	MostCommonOccurence<Checksums> mc;
	for(auto& sector: m.sectors)
	{
		mc.compute(sector.checks);
		//std::cout << boost::format("mc.count %i, checks = %s\n") % mc.count % sector.checks.occurences();
		s.minMC = std::min(s.minMC, mc.count);
		s.maxMC = std::max(s.maxMC, mc.count);
		mcc[0] += 1;
		mcc[1] += mc.count;
		nrReads[0] += 1;
		nrReads[1] += sector.checks.sum();
	}
	s.meanMC = mcc[1]/mcc[0];
	s.meanNrReads = nrReads[1]/nrReads[0];

	return s;
}


//-- SectorDb::AudioReader


AudioReader::HardwareInfo SectorDb::GetHardwareInfo()
{
	return m.hw;
}


Toc SectorDb::GetToc(bool /*isrc*/, bool /*cdtext*/)
{
	return m.toc;
}


int SectorDb::SizeInSectors()
{
	return static_cast<int>(m.sectors.size());
}


int SectorDb::ReadAudio(int LBA, buffer_view<int16_t> destination, int nrFrames)
{
	for(int n=0; n < nrFrames; n++)
	{
		buffer_view<int16_t> src_n = Audio(m.sectors.at(LBA+n));
		buffer_view<int16_t> dst_n{destination.data() + n*(int)CddaFrame::Audio/2, (int)CddaFrame::Audio/2};
		assert(dst_n.end() <= destination.end());
		assert(dst_n.size() == src_n.size());
		std::copy(src_n.begin(), src_n.end(), dst_n.begin());
	}
	return 0;
}


/// Get the audio part
buffer_view<int16_t> Audio(const SectorDb::Sector& s)
{
	return buffer_view<int16_t>{(int16_t*)s.data.data(), (int)CddaFrame::Audio/2};
}


/// Get the raw subchannel
buffer_view<uint8_t> SubChannel(SectorDb::Sector& s)
{
	uint8_t* sub = &s.data[(int)CddaFrame::Audio];
	return buffer_view<uint8_t>{sub, (int)CddaFrame::RawSubChannel};
}
