#pragma once

#include <algorithm>

#include <boost/serialization/version.hpp>

#include "../gpc.h"
#include "../buffer_view.h"
#include "../grab_export.h"


namespace boost{ namespace serialization {
	class access;
} }


template<typename T>
class OccurenceList
{
public:
	typedef std::pair<int, T> occurence_t;
	typedef std::vector<occurence_t> occurences_t;

	void push_back(const T& v)
	{
		auto it = std::find_if(m.begin(), m.end(), [&v](const occurence_t& occ){
			return occ.second == v;
		});
		if(it == m.end())
			m.push_back({1, v});
		else
			it->first++;
	}

	const occurences_t& occurences() const
	{
		return m;
	}

	int sum() const
	{
		int s = 0;
		for(auto& occ: m)
			s += occ.first;
		return s;
	}

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int)
	{
		ar & m;
	}

private:
	occurences_t m;
};

class SectorDbProgress
{
public:
	/// Reports progress, should return false to cancel current work
	virtual bool OnSectorProgress(int nrSectors, int lba, double pct) = 0;
};

class SectorDbProgressStdOut: public SectorDbProgress
{
	bool OnSectorProgress(int nrSectors, int lba, double pct) override;
};

/// Database of recorded sectors and CRC's of that.
/// Keeps track of all CRC's seen per sector, keeps the data
/// of the most common CRC
class GRAB_EXPORT SectorDb : public AudioReader
{
public:
	struct Checksums
	{
		Checksums();

		uint32_t crc;				///< CRC of the whole audio frame
		uint8_t blockErrorMask;		///< Logical or of all C2 checks
	};

	struct Sector
	{
		std::array<uint8_t, (int)CddaFrame::Audio + (int)CddaFrame::RawSubChannel> data;
		Checksums check;				///< Checksum of current data
		//std::vector<Checksums> checks;	///< Checksums of audio, over multiple recordings
		OccurenceList<Checksums> checks;
	};

	struct GRAB_EXPORT Statistics
	{
		Statistics();

		int minMC;	/// minimum of common CRC's over all sectors
		int maxMC;	/// maximum of common CRC's over all sectors
		double meanMC;
		double meanNrReads;
	};

	/// Create a new db. TOC is optional, but since it's slow to read,
	/// It can be passed to prevent further reading by SectorDb.
	SectorDb(int nrSectors = 0, Toc* pToc=nullptr);

	SectorDb(const std::string& fname);

	~SectorDb();

	/// Add audio only (no subframe), at sector \p lba.
	void AddAudio(int lba, buffer_view<int16_t> audio);

	/// Re-reads each sector for which not at least 'minimumCommonCrcCount' common CRC's have been seen/
	/// returns nr of sectors read during this pass
	int Refine(AudioReader& reader, int minimumCommonCrcCount = 2, SectorDbProgress* progress=nullptr);

	/// Get sector at index lba
	const Sector& operator[](int lba);

	/// Save to file
	void Save(const std::string& fname);

	Statistics GetStatistics();

	///-- AudioReader --
	HardwareInfo GetHardwareInfo() override;
	Toc GetToc(bool isrc, bool cdtext) override;
	int SizeInSectors() override;
	int ReadAudio(int LBA, buffer_view<int16_t> dst, int nrFrames) override;

	/// Internal data, structure made public for serialization
	struct Data
	{
		AudioReader::HardwareInfo hw;
		Toc toc;
		std::vector<Sector> sectors;
	};
private:
	Data m;
};

BOOST_CLASS_VERSION(SectorDb::Checksums, 0)
BOOST_CLASS_VERSION(SectorDb::Sector, 0)

/// Get the audio part
buffer_view<int16_t> Audio(const SectorDb::Sector& s);

/// Get the raw subchannel
buffer_view<uint8_t> SubChannel(SectorDb::Sector& s);
