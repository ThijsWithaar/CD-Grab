#pragma once

#include <string>

#include <cdgrab/gpc.h>
#include <cdgrab/file/sectorDb.h>

#include <cdgrab/DiscInfoDatabase.h>


class GrabberProgress
{
public:
	enum Action
	{
		TableOfContents,
		DiscId,
		PrepareSectorDb,
		Reading,
		SaveSectorDb,
		Process
	};

	virtual bool OnProgress(Action action, double progress) = 0;
};

std::ostream& operator<<(std::ostream& os, GrabberProgress::Action act);

class Grabber
{
public:
	Grabber();

	// Loads sectordb, re-reads cd if necessary
	AudioReader& Grab(AudioReader& cdda, GrabberProgress* progress=nullptr);

	AudioReader& LoadSectorDb(std::string fname);

	// Encode the SectorDb into cue/flac
	void Encode(GrabberProgress* progress = nullptr);
private:
	DiscInfo GetInfo(AudioReader& src, GrabberProgress* progress=nullptr);

	Toc m_toc;
	DiscInfoDatabase m_discDb;
	SectorDb m_sdb;
	std::string m_baseName;
};
