#include <iostream>
#include <fstream>

#include <boost/format.hpp>

#include <boost/filesystem.hpp>

#include <cmath>

#include <cdgrab/cdda.h>
#include <cdgrab/DiscInfoDatabase.h>
#include <cdgrab/file/cue.h>
#include <cdgrab/file/flac.h>
#include <cdgrab/file/opus.h>
#include <cdgrab/file/sectorDb.h>

#include "MainWindow.h"
#include "grabber.h"



void PrintDeviceInfo(CDDA& cdda)
{
	auto hwi = cdda.GetHardwareInfo();
	std::cout << "Vendor           : '" << hwi.vendor << "'\n";
	std::cout << "Model            : '" << hwi.model << "'\n";
	std::cout << "Revision         : '" << hwi.revision << "'\n";

	std::cout << "Drive status     : " << cdda.GetDriveStatus() << "\n";
	std::cout << "Disc status      : " << cdda.GetDiscStatus() << "\n";

	auto perf = cdda.GetPerformance();
	std::cout << "Read speed       : " << perf.read_kBs << " kB/s\n";
	std::cout << "Write speed      : " << perf.write_kBs << " kB/s\n";

	perf = cdda.GetMinimumWriteSpeed();
	std::cout << "Min. Read speed  : " << perf.read_kBs << " kB/s\n";
	std::cout << "Min. Write speed : " << perf.write_kBs << " kB/s\n";
}


std::ostream& operator<<(std::ostream& os, DiscInfo di)
{
	os << di.artist << " - " << di.title << "\n";
	for(auto tr: di.track)
		os << "\t" << tr.songName << "\n";
	return os;
}


class GrabberProgressStdout: public GrabberProgress
{
public:
	bool OnProgress(Action action, double progress) override
	{
		std::cout << boost::format("\rAction: %-13s %6.2f%%") % action % progress;
		if(progress == 100.)
			std::cout << "\n";
		std::cout << std::flush;
		return true;
	}
};


int main(int argc, char* argv[])
try
{
#if 0
	auto sectorFname = "O.A.R. - The Mighty/O.A.R. - The Mighty - 019075940562.sdb";
	//auto sectorFname = "Metallica - Load/Metallica - Load - 073145326182.sdb";
	//auto sectorFname = "O.A.R. - King/O.A.R. - King.sdb";
	//auto sectorFname = "Skunk Anansie/Skunk Anansie - Paranoid & Sunburnt - 724384091125.sdb";

	Grabber grabber;
	auto& sdb = grabber.LoadSectorDb(sectorFname);

	Toc toc = sdb.GetToc();
	DiscInfoDatabase dib;
	auto tags = dib.Query(toc);
	std::cout << "tags = " << tags << "\n";

	grabber.Encode();
#elif 1
	QApplication app(argc, argv);
	MainWindow main(nullptr);
	main.show();
	return app.exec();
#else
	auto devs = GetDevices();
	if(devs.empty())
		return -1;

	for(auto devName: {devs[0]})
	{
		std::cout << "Found device : " << devName << "\n";
		CDDA cdda(devName, CDDA::CommunicationChannel::SCSI);

		//PrintDeviceInfo(cdda);
		cdda.GetCapabilities();

		int maxLba = cdda.SizeInSectors();
		auto maxMsf = ToMSF(maxLba);
		std::cout << boost::format("Sector Size  : %i (%2i:%02i)\n") % maxLba % maxMsf.minute % maxMsf.second;
		//std::cout << "Sector Size HL: " << cdda.SizeInSectorsHl() << "\n";

#if 0
		auto upc = cdda.UniversalProductCode();
		std::cout << boost::format("UPC: %s, from A-frame %i\n") % upc.second % upc.first << "\n";

		int lba = 5120;
		//for(int lba = 0; lba < maxLba; lba += 256)
		{
			int e2 = cdda.ReadCd(lba, 1, false);
			if(e2 > 0)
				std::cout << boost::format("lba %8i: c2 %i\n") % lba % e2;
		}
#endif

#if 1
		Grabber grabber;
		GrabberProgressStdout progress;

		auto& sdb = grabber.Grab(cdda, &progress);
		grabber.Encode(&progress);
		cdda.Eject();
#endif

	}
#endif
	return 0;
}
catch(std::exception& e)
{
	std::cerr << e.what() << "\n";
	return -1;
}
