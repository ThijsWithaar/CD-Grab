#pragma once

/*
CDDB Protocol implementation
http://ftp.freedb.org/pub/freedb/latest/CDDBPROTO
*/


#include <map>
#include <string>
#include <sstream>

#include <boost/asio.hpp>

#include "gpc.h"
#include "DiscInfoDatabase.h"


/// The socket connection, gets the whole response as a string
class CddbConnection
{
public:
	CddbConnection(std::string host, int port = 80);

	std::stringstream Get(std::string req);

private:
	boost::asio::io_context io_context;
	boost::asio::ip::tcp::resolver m_resolver;
	boost::asio::ip::tcp::resolver::results_type m_resolved;
	boost::asio::ip::tcp::socket m_socket;
	std::string m_host;
};

/// Build up a query for a disc, based on the TOC
struct CddbQuery
{
public:
	CddbQuery(const Toc& toc);

	operator std::string() const;

	uint32_t disc_id;
	std::vector<int> frame_offsets;
	int total_time;
	int nrTracks;
private:
	static int sum(int n);
};


/// The main interface
class Cddb
{
public:
	struct QueryResult
	{
		bool operator<(const QueryResult& o) const;

		std::string category;
		uint32_t id;
		std::string title;
		bool exact;
	};

	struct Result
	{
		QueryResult query;
		DiscInfo info;
	};

	Cddb();

	void LoadDatabase(std::string path);

	/// Queries the database, returns a list of ID's
	std::vector<QueryResult> Query(const Toc& toc);

	/// Gets the data via the CDDB protocol
	DiscInfo GetDiscById(QueryResult qr);

	/// Gets the data via http
	Result Get(std::string category, const Toc& toc);

	Result ParseXmcd(std::istream& is);

private:
	std::string host, path;
	std::string user, hostname, clientname, client_version;
	int proto;

	std::map<int, QueryResult> m_queries;
	std::map<QueryResult, DiscInfo> m_infos;
	std::string m_localPath;
};
