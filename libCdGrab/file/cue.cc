#include <cdgrab/file/cue.h>

#include <fstream>
#include <string_view>

#include <boost/format.hpp>



template<size_t N>
std::string_view sv(const std::array<char,N>& s)
{
	if(s[0] == '\0')
		return std::string_view(nullptr, 0);
	return std::string_view{s.data(), s.size()};
}


void Write(const Cue& cue, const DiscInfo& tags, const std::string& cueFilename)
{
	std::ofstream f(cueFilename);
	f << "FILE \"" << cue.datafilename << "\" " << cue.format << "\n";
	if(tags.year > 0)
		f << "REM DATE " << tags.year << "\n";
	if(!tags.artist.empty())
		f << "REM PERFORMER \"" << tags.artist << "\"\n";
	if(!tags.title.empty())
		f << "REM TITLE \"" << tags.title << "\"\n";
	if(!cue.upc.empty())
		f << "REM DISCID " << cue.upc << "\n";
	for(auto& track: cue.tracks)
	{
		if(track.track > 99)
			continue;
		auto pTagTrack = (track.track <= tags.track.size()) ? &tags.track[track.track-1] : nullptr;

		f << boost::format("\tTRACK %02i AUDIO\n") % track.track;
		if(pTagTrack && !pTagTrack->artist.empty())
			f << boost::format("\t\tPERFORMER \"%s\"\n") % pTagTrack->artist;
		if(pTagTrack && !pTagTrack->songName.empty())
			f << boost::format("\t\tTITLE \"%s\"\n") % pTagTrack->songName;
		auto isrc = sv(track.isrc);
		if(!isrc.empty())
			f << boost::format("\t\tISRC %s\n") % isrc;
		f << boost::format("\t\tINDEX 01 %02i:%02i:%02i\n") % track.minute % track.second % track.frame;
	}
}


/// For a binary dump of the data
void WriteBinaryCue(std::string cueFilename, std::string dataFilename)
{
	std::ofstream f(cueFilename);
	f << boost::format(
	"FILE \"%s\" BINARY\n"
	"\tTRACK 01 MODE1/2352\n"
    "\t\tINDEX 01 00:00:00\n") % dataFilename;
}
