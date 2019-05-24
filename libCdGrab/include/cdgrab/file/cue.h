#pragma once

#include <string>

#include "../DiscInfoDatabase.h"
#include "../gpc.h"
#include "../grab_export.h"


struct Cue : public Toc
{
	std::string datafilename;
	std::string format;
};

/// Audio as single file, cue with track info
GRAB_EXPORT void Write(const Cue& cue, const DiscInfo& tags, const std::string& cueFilename);

/// For a binary dump of the audio data, no track indices
void WriteBinaryCue(std::string cueFilename, std::string dataFilename);
