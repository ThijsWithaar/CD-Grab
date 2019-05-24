#include <cdgrab/cddb.h>

#include <iostream>
#include <cstdlib>
#include <fstream>
//#include <filesystem>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>


const char* ws = " \t\n\r\f\v";


// trim from end of string (right)
inline std::string& rtrim(std::string& s, const char* t = ws)
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from beginning of string (left)
inline std::string& ltrim(std::string& s, const char* t = ws)
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from both ends of string (right then left)
inline std::string& trim(std::string& s, const char* t = ws)
{
    return ltrim(rtrim(s, t), t);
}


template<typename T>
T join(const T& sep, const std::vector<T>& els)
{
	T r;
	if(els.empty())
		return r;
	auto it = els.begin();
	r += *it;
	for(++it; it != els.end(); ++it)
		r += sep + *it;
	return r;
}


//-- CddbConnection --


CddbConnection::CddbConnection(std::string host, int port):
	m_socket(io_context),
	m_resolver(io_context),
	m_host(host)
{
	std::string ports = std::to_string(port);
	m_resolved = m_resolver.resolve(host, ports);
	boost::asio::connect(m_socket, m_resolved);
}


std::stringstream CddbConnection::Get(std::string req)
{
	using boost::asio::ip::tcp;
	if(!m_socket.is_open())
		boost::asio::connect(m_socket, m_resolved);

	boost::asio::streambuf request;
	std::ostream request_stream(&request);
	request_stream << "GET " << req << "\r\n";
	request_stream << "Host: " << m_host << "\r\n";
	request_stream << "Accept: */*\r\n";
	request_stream << "Connection: close\r\n\r\n";

	boost::asio::write(m_socket, request);

	boost::system::error_code error;
	boost::asio::streambuf response;
	std::stringstream retss;

	while (boost::asio::read(m_socket, response, boost::asio::transfer_at_least(1), error))
		retss << &response;
	if (error != boost::asio::error::eof)
		throw boost::system::system_error(error);

	m_socket.close();
	return retss;
}


//-- CddbQuery ---


CddbQuery::CddbQuery(const Toc& toc)
{
	const int frames_sec = 75;
	uint32_t checksum = 0;
	auto& tracks = toc.tracks;
	// Including the leadout:
	for(int i = 0; i < tracks.size(); i++)
	{
		auto& t = tracks[i];
		auto off = ToFrame(t);
		if(i < tracks.size()-1)
			checksum += sum(off/frames_sec);
		frame_offsets.push_back(off);
	}

	total_time = frame_offsets.back()/frames_sec - frame_offsets.front()/frames_sec;
	uint32_t last = (uint32_t)tracks.size() -1;
	uint32_t tc = (checksum % 0xff) << 24;
	disc_id = tc | total_time << 8 | last;
}


CddbQuery::operator std::string() const
{
	int nValidTrack = (int)frame_offsets.size() - 1;
	std::string query = (boost::format("%08lx+%d+") % disc_id % nValidTrack).str();
	for(int i = 0; i < frame_offsets.size() - 1; i++)
	{
		auto frame_offset = frame_offsets[i];
		query += (boost::format("%d+") % frame_offset).str();
	}
	query += std::to_string(total_time);
	return query;
}


int CddbQuery::sum(int n)
{
	int r = 0;
	while(n>0)
	{
		r += n%10;
		n /= 10;
	}
	return r;
}


//-- Cddb --


bool Cddb::QueryResult::operator<(const QueryResult& o) const
{
	if(id != o.id)
		return id < o.id;
	if(category != o.category)
		return category < o.category;
	return title < o.title;
}


Cddb::Cddb()
{
	host = "freedb.freedb.org";
	path = "/~cddb/cddb.cgi";
	user = "user";
	hostname = "host";
	clientname = "cdgrab";
	client_version = "0.26";
	proto = 5;
}


void Cddb::LoadDatabase(std::string path)
{
	namespace fs = boost::filesystem;
	fs::path pt(path);
	fs::directory_iterator end;
	for (fs::directory_iterator it(pt); it != end; ++it)
	{
		if(fs::is_regular_file(*it))
		{	// Root folder: assume category tag is set
			std::ifstream fxmcd(it->path().string());
			auto cp = ParseXmcd(fxmcd);
			cp.query.exact = true;
			m_queries[cp.query.id] = cp.query;
			m_infos[cp.query] = cp.info;
		}
		else if(fs::is_directory(*it))
		{	// Folders are the categories
			std::string category = fs::basename(*it);
			for (fs::directory_iterator it2(*it); it2 != end; ++it2)
			{
				std::ifstream fxmcd(it2->path().string());
				auto cp = ParseXmcd(fxmcd);
				cp.query.exact = true;
				cp.query.category = category;
				m_queries[cp.query.id] = cp.query;
				m_infos[cp.query] = cp.info;
			}
		}
	}

	m_localPath = path;
}


Cddb::QueryResult ParseQr(std::vector<std::string>& els)
{
	Cddb::QueryResult qr;
	qr.category = els[0];
	qr.id = static_cast<uint32_t>(std::stoll(els[1], nullptr, 16));
	els.erase(els.begin(), els.begin()+2);
	qr.title = join<std::string>(" ", els);
	return qr;
}


enum CddbResponseCode
{
	SingleMatch = 200,
	ExactMatches = 210,
	InexactMatches = 211
};


std::vector<Cddb::QueryResult> Cddb::Query(const Toc& toc)
{
	CddbQuery cq(toc);
	std::string cqs = cq;

	// Local first
	std::vector<QueryResult> ids;
	if(m_queries.count(cq.disc_id) > 0)
		ids.push_back(m_queries.at(cq.disc_id));

	// Then remote, appended
	std::string req = (boost::format("%s?cmd=cddb+query+%s&hello=%s+%s+%s+%s&proto=%i") % path % cqs % user % hostname % clientname % client_version % proto).str();

	CddbConnection con(host);
	std::stringstream resp = con.Get(req);

	std::string header;
	std::getline(resp, header,'\n');
	rtrim(header);

	std::vector<std::string> headers;
	boost::split(headers, header, boost::is_any_of(" "));
	int code = std::stoi(headers[0]);

	std::vector<std::string> els;
	if(code == SingleMatch)
	{
		// Exact match
		boost::split(els, header, boost::is_any_of(" "));
		els.erase(els.begin());	// remove code entry
		auto qr = ParseQr(els);
		qr.exact = true;
		ids.push_back(qr);
		m_queries[qr.id] = qr;
	}
	else if(code == ExactMatches || code == InexactMatches)
	{
		std::string line;
		while(std::getline(resp, line, '\n'))
		{
			if(line[0] == '.')
				break;
			rtrim(line);
			boost::split(els, line, boost::is_any_of(" "));
			auto qr = ParseQr(els);
			qr.exact = code == ExactMatches;
			ids.push_back(qr);
			if(code == ExactMatches)
				m_queries[qr.id] = qr;
		}
	}

	return ids;
}


Cddb::Result Cddb::ParseXmcd(std::istream& resp)
{
	QueryResult qr;
	DiscInfo di;

	bool inTrackOffsets = false;
	std::vector<int> offsets;

	std::string line;
	std::vector<std::string> els;
	while(std::getline(resp, line, '\n'))
	{
		if(line[0] == '.')
			break;
		else if(line[0] == '#')
		{
			if(boost::starts_with(line, "# Track frame offsets"))
				inTrackOffsets = true;
			else if(inTrackOffsets)
			{
				trim(line, " \t\r\n");
				if(line.size() < 2)
					inTrackOffsets = false;
				else
				{
					trim(line, "# \t");
					offsets.push_back(std::stoi(line));
				}
			}
		}
		else
		{
			boost::split(els, line, boost::is_any_of("="));
			if(els.size() != 2)
				continue;
			auto key = trim(els[0]);
			auto val = trim(els[1], " \r\n");
			if(key == "DISCID")
			{
				qr.id = static_cast<uint32_t>(std::stoll(val, nullptr, 16));
			}
			if(key == "DYEAR")
			{
				di.year = std::stoi(val);
			}
			if(key == "CATEGORY" || key == "#CATEGORY")
			{
				qr.category = val;
			}
			else if(key == "DTITLE")
			{
				qr.title = val;
				boost::split(els, val,  boost::is_any_of("/"));
				if(els.size() >= 2)
				{
					di.artist = rtrim(els[0]);
					di.title = ltrim(els[1]);
				}
			}
			else if(boost::starts_with(key, "TTITLE"))
			{
				TrackInfo ti;
				ti.songName = val;
				di.track.push_back(ti);
			}
		}
	} // getline

	for(size_t n=0; n < std::min(di.track.size(), offsets.size()); n++)
	{
		di.track[n].start = FromFrame(offsets[n]);
	}
	if(!offsets.empty())
		di.leadout = FromFrame(offsets.back());

	return {qr, di};
}


DiscInfo Cddb::GetDiscById(QueryResult qr)
{
	if(m_infos.count(qr) > 0)
		return m_infos.at(qr);

	std::string req = (boost::format("%s?cmd=cddb+read+%s+%08x&hello=%s+%s+%s+%s&proto=%i") % path % qr.category % qr.id % user % hostname % clientname % client_version % proto).str();

	CddbConnection con(host);
	std::stringstream resp = con.Get(req);

	std::string header;
	std::getline(resp, header,'\n');
	rtrim(header);
	auto bodyStart = resp.tellg();

	std::vector<std::string> headers;
	boost::split(headers, header, boost::is_any_of(" "));
	int code = std::stoi(headers[0]);

	if(!m_localPath.empty() && code == 210)
	{
		namespace fs = boost::filesystem;
		auto pt = fs::path(m_localPath) / qr.category;
		fs::create_directory(pt);

		std::string fname = (boost::format("%08x") % qr.id).str();
		fname = (pt / fname).string();
		std::cout << "CDDB cache to " << fname << "\n";
		std::ofstream fxcmd(fname);

		while(std::getline(resp, header,'\n'))
		{
			if(!header.empty() && header[0] == '.')
				break;
			fxcmd << header << "\n";
		}
		// Restore for ParseXmcd
		resp.clear();
		resp.seekg(bodyStart, std::ios::beg);
	}

	return ParseXmcd(resp).info;
}


Cddb::Result Cddb::Get(std::string category, const Toc& toc)
{
	CddbQuery cq(toc);
	std::string url = (boost::format("/%s/%08x") % category % cq.disc_id).str();

	CddbConnection con(host);
	std::stringstream resp = con.Get(url);

	Cddb::Result res = ParseXmcd(resp);
	return res;
}
