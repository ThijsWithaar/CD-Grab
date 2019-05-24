#include <catch2/catch.hpp>

#include <cdgrab/cddb.h>
//#include <cdgrab/file/sectorDb.h>


Toc BuildToc(std::vector<int> frame_offsets)
{
	Toc toc;
	TocEntry e;
	e.minute = e.second = 0;
	for(auto frame_offset: frame_offsets)
	{
		e.frame = frame_offset;
		toc.tracks.push_back(e);
	}
	return toc;
}


TEST_CASE("CDDB ID")
{
	SECTION("Sepultura - Arise (Single)")
	{
		std::vector<int> frame_offsets = {183, 15258, 36715, 50118};
		auto toc = BuildToc(frame_offsets);
		CddbQuery cdq(toc);
		CHECK(cdq.disc_id == 0x1c029a03);
	}

	SECTION("Skunk Anansie - Paranoid & Sunburnt")
	{
		auto frame_offsets = {
			182, 16997, 33812,
			49782, 68322, 83477,
			104247, 123227, 139110,
			162512, 180587, 199232};
		auto toc = BuildToc(frame_offsets);

		CddbQuery cdq(toc);
		CHECK(cdq.disc_id == 0x850a5e0b);
	}

	SECTION("Dire Straits - Making Movies")
	{
		auto frame_offsets = {183, 36835, 63420, 91830, 114588, 136275, 151233, 169690};
		auto toc = BuildToc(frame_offsets);

		CddbQuery cdq(toc);
		CHECK(cdq.disc_id == 0x5308d407);
	}

	//std::string fnSbd = "Dire Straits - Making Movies/Dire Straits - Making Movies - 325918000502.sdb";
	//SectorDb db(fnSbd);
	//auto toc = db.GetToc();
}
