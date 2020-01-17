#include <cdgrab/file/nrg.h>

#include <iostream>
#include <fstream>

#include <boost/format.hpp>
#include <boost/program_options.hpp>


struct Options
{
	std::string fnInput;
};


Options Parse(int argc, char* argv[])
{
	namespace po = boost::program_options;
	po::options_description desc("Allowed options");

	bool help;
	Options ret;

	desc.add_options()
		("help", po::bool_switch(&help), "show the help message")
		("input", po::value<std::string>(&ret.fnInput), "input image filename")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (help || argc == 1)
		std::cout << desc << "\n";

	return ret;
}


int main(int argc, char* argv[])
{
	Options opt = Parse(argc, argv);
	if(opt.fnInput.empty())
		return -1;

	std::cout << "Opening: " << opt.fnInput << "\n";

	Nrg nrg(opt.fnInput);
	boost::iostreams::mapped_file_source mfile(opt.fnInput);

	int trackIndex = 1;
	for(const auto& track: nrg.Tracks())
	{
		std::string fnOut = (boost::format("%02i.dts") % (trackIndex++)).str();

		const char* pStart = mfile.data() + track.index1;
		ptrdiff_t len = track.indexEnd - track.index1;

		std::cout << boost::format("Writing %s, offset 0x%08x, length 0x%08x\n") % fnOut % track.index1 % len;

		std::ofstream fDst(fnOut, std::ios_base::binary);
		fDst.write(pStart, len);
	}

	return 0;
}
