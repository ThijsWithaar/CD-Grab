#include <boost/asio.hpp>


#include <cdgrab/DiscInfoDatabase.h>

#include <iostream>
#include <fstream>
#include <sstream>


#include <boost/format.hpp>
#include <boost/filesystem.hpp>


#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include <boost/serialization/array.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>

#include <boost/functional/hash.hpp>

//#include <boost/beast.hpp>
//#include <boost/asio/ssl.hpp>
//#include <boost/asio/connect.hpp>
//#include <boost/asio/ip/tcp.hpp>
//#include <boost/asio/read_until.hpp>

//#include <boost/beast/core.hpp>
//#include <boost/beast/http.hpp>
//#include <boost/beast/version.hpp>

//#include <boost/beast/ssl.hpp>
//#include <boost/asio/ssl/error.hpp>
//#include <boost/asio/ssl/stream.hpp>

#include <cdgrab/cddb.h>


std::size_t hash_value(MSF const& b)
{
	std::size_t seed = 0;
	boost::hash_combine(seed, b.frame);
	boost::hash_combine(seed, b.minute);
	boost::hash_combine(seed, b.second);
	return seed;
}


std::size_t hash_value(const Toc& toc)
{
	std::size_t seed = toc.tracks.size();
	for(auto& t: toc.tracks)
		 boost::hash_combine(seed, MSF(t));
	return seed;
}


std::size_t hash_value(const DiscInfo& di)
{
	std::size_t seed = di.track.size() + 1;	// + leadout
	for(auto t: di.track)
        boost::hash_combine(seed, t.start);
	boost::hash_combine(seed, di.leadout);
	return seed;
}


DiscInfoId::DiscInfoId():
	id(0)
{
}


DiscInfoId::DiscInfoId(const Toc& toc):
	id(static_cast<uint32_t>(hash_value(toc)))
{
}


DiscInfoId::DiscInfoId(const DiscInfo& di):
	id(static_cast<uint32_t>(hash_value(di)))
{
}


template<typename Ar>
void serialize(Ar& ar, MSF& msf, unsigned int version)
{
	ar & BOOST_SERIALIZATION_NVP(msf.minute) &
		BOOST_SERIALIZATION_NVP(msf.second) &
		BOOST_SERIALIZATION_NVP(msf.frame);
}


template<typename Ar>
void serialize(Ar& ar, TrackInfo& info, unsigned int version)
{
	CheckSerializationVersion<TrackInfo>(version);
	ar & BOOST_SERIALIZATION_NVP(info.start);
	ar & BOOST_SERIALIZATION_NVP(info.artist) &
		BOOST_SERIALIZATION_NVP(info.songName) &
		BOOST_SERIALIZATION_NVP(info.isrc);
}


template<typename Ar>
void serialize(Ar& ar, DiscInfo& info, unsigned int version)
{
	CheckSerializationVersion<DiscInfo>(version);
	ar & BOOST_SERIALIZATION_NVP(info.artist) &
		BOOST_SERIALIZATION_NVP(info.title) &
		BOOST_SERIALIZATION_NVP(info.year);
	ar & BOOST_SERIALIZATION_NVP(info.upc) &
		BOOST_SERIALIZATION_NVP(info.leadout) &
		BOOST_SERIALIZATION_NVP(info.track);
}


TrackInfo::TrackInfo():
	isrc{0}
{
}


DiscInfoDatabase::DiscInfoDatabase():
	m_cddb(std::make_unique<Cddb>())
{
	try
	{
		std::ifstream f("cddb2.cache.bin", std::ios::binary);
		boost::archive::binary_iarchive ar(f);
		ar & m_cache;
	} catch(std::exception&) {}

	namespace fs = boost::filesystem;
	fs::path pt = ".";
	for(int i=0; i < 4; i++)
	{
		fs::path ptx = pt / "xmcd";
		if(fs::is_directory(ptx))
		{
			m_cddb->LoadDatabase(ptx.string());
			break;
		}
		pt = ".." / pt;
	}
}


DiscInfoDatabase::~DiscInfoDatabase()
{
	try
	{
		//Save();	// Done on every update, not required here
	} catch(std::exception&) {}
}


void DiscInfoDatabase::Save()
{
	std::ofstream f("cddb2.cache.bin", std::ios::binary);
	boost::archive::binary_oarchive ar(f);
	ar & m_cache;
	{
		std::ofstream f("cddb2.cache.xml");
		boost::archive::xml_oarchive ar(f);
		ar & boost::serialization::make_nvp("cache", m_cache);
	}
}


TrackInfo CreateTrackInfo(const TocEntry& te)
{
	TrackInfo r;
	r.start = te;
	r.artist = te.performer;
	r.songName = te.title;
	r.isrc = te.isrc;
	return r;
}


DiscInfo CreateDiscInfo(const Toc& toc)
{
	DiscInfo r;
	r.artist = toc.performer;
	r.title = toc.title;
	r.year = 0;
	r.upc = toc.upc;
	for(auto& tr: toc.tracks)
		if(tr.track < 99)	// Don't store the leadout
			r.track.push_back(CreateTrackInfo(tr));
	r.leadout = toc.tracks.back();
	return r;
}


bool DiscInfoDatabase::Has(DiscInfoId id)
{
	return m_cache.count(id.id) > 0;
}


boost::optional<DiscInfo> DiscInfoDatabase::Get(DiscInfoId id)
{
	if(m_cache.count(id.id))
	{
		auto di = m_cache.at(id.id);
		return di;
	}
	return {};
}


void DiscInfoDatabase::Store(const DiscInfo& di)
{
	boost::hash<DiscInfo> hasher;
	uint32_t hash = (uint32_t)hasher(di);
	m_cache[hash] = di;

	try
	{
		Save();
	} catch(std::exception&) {}
}


DiscInfo DiscInfoDatabase::Query(const Toc& toc)
{
	auto ids = m_cddb->Query(toc);
	//for(auto id: ids) std::cout << boost::format("CDDB: %s/%08x: %s\n") % id.category % id.id % id.title;

	DiscInfo di;
	if(ids.empty())
		return di;

	di = m_cddb->GetDiscById(ids.front());

	// Merge toc and cddb data:
	di.upc = toc.upc;
	for(size_t t=0; t < std::min(toc.tracks.size(), di.track.size()); t++)
	{
		di.track[t].start = toc.tracks[t];
		di.track[t].isrc = toc.tracks[t].isrc;
	}
	di.leadout = toc.tracks.back();

	boost::hash<DiscInfo> hasher;
	uint32_t hash = (uint32_t)hasher(di);

	boost::hash<Toc> hasher2;
	uint32_t hash2 = (uint32_t)hasher2(toc);
	assert(hash == hash2 && "Lookup won't work if hashes don't match");

	m_cache[hash] = di;
	try
	{
		Save();
	} catch(std::exception&) {}

	return di;
}


//-- MusicBrainz --


class MusicBrainz
{
public:
	void Query(const Toc& toc);

private:
	std::string BuildQuery(const Toc& toc);
};



// https://wiki.musicbrainz.org/Development/XML_Web_Service/Version_2
void MusicBrainz::Query(const Toc& toc)
{
	const std::string url = "https://musicbrainz.org";
	auto query = BuildQuery(toc);

	// To get a JSON response you can either set the Accept header to "application/json"

	// Set a user-agent header: Application name/<version> ( contact-email )
}


std::string MusicBrainz::BuildQuery(const Toc& toc)
{
	std::stringstream ss;

	// First track (always 1)
	ss << "/ws/2/discid/-?toc=1";

	// total number of tracks
	ss << "+" << (toc.tracks.size() - 1);

	// sector offset of the leadout (end of the disc)
	auto leadout = toc.tracks.back();
	ss << "+" << ToLBA(leadout);

	// a list of sector offsets for each track, beginning with track 1 (generally 150 sectors)
	for(auto track = toc.tracks.begin(); track < toc.tracks.end() - 1; ++track)
		ss << "+" << ToLBA(leadout);

	return ss.str();
}
