#pragma once

#include <map>
#include <memory>
#include <vector>

#include <boost/serialization/version.hpp>

#include <cdgrab/grab_export.h>
#include <cdgrab/gpc.h>


class Cddb;


struct GRAB_EXPORT TrackInfo
{
	TrackInfo();

	MSF start;
	std::string artist;
	std::string songName;
	std::array<char,12> isrc;
};
BOOST_CLASS_VERSION(TrackInfo, 2)


struct DiscInfo
{
	std::string artist, title;
	int year;
	std::string upc;
	MSF leadout;
	std::vector<TrackInfo> track;
};
BOOST_CLASS_VERSION(DiscInfo, 1)

GRAB_EXPORT DiscInfo CreateDiscInfo(const Toc& toc);

struct GRAB_EXPORT DiscInfoId
{
	DiscInfoId();
	DiscInfoId(const Toc& toc);
	DiscInfoId(const DiscInfo& di);

	uint32_t id;
};

class GRAB_EXPORT DiscInfoDatabase
{
public:
	DiscInfoDatabase();
	~DiscInfoDatabase();

	/// Returns true if id is in local cache.
	bool Has(DiscInfoId id);

	/// Store in the db
	void Store(const DiscInfo& di);

	/// Retrieve from database, using only MSF track offsets
	boost::optional<DiscInfo> Get(DiscInfoId id);

	/// Query external sources, updates database with result.
	/// Merges info from toc with external sources
	DiscInfo Query(const Toc& toc);

private:
	void Save();

	std::unique_ptr<Cddb> m_cddb;
	std::map<uint32_t, DiscInfo> m_cache;
};
