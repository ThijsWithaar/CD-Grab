#include <iostream>

#include <catch2/catch.hpp>

#include <cdgrab/file/sectorDb.h>


template<typename T, size_t N>
std::ostream& operator<<(std::ostream& os, std::array<T,N> arr)
{
	auto it = arr.begin();
	os << *it;
	for(++it; it != arr.end(); ++it)
		os << "," << *it;
	return os;
}


/// Reader which can be controlled from the testing interface.
/// The first byte of each sector is controlled by a callback function
class MockReader: public AudioReader
{
public:
	/// This defines the data of the first byte of each sector
	typedef std::function<uint8_t(int lba)> data_fcn_t;

	MockReader(int nrSectors, data_fcn_t data):
		m_nrSectors(nrSectors),
		m_data(data),
		buffer{0}
	{
	}

	HardwareInfo GetHardwareInfo() override
	{
		return hwinfo;
	}

	Toc GetToc(bool isrc, bool cdtext) override
	{
		return toc;
	}

	int SizeInSectors() override
	{
		return m_nrSectors;
	}

	int ReadAudio(int LBA, buffer_view<int16_t> dst, int nrFrames) override
	{
		assert(nrFrames == 1 && "Multi-frame not supported");
		std::fill(dst.begin(), dst.end(), 0);
		dst[0] = m_data(LBA);
		//std::cout << boost::format("ReadAudio sector %i, value %i\n") % LBA % dst.data()[0];
		return 0;
	}

public:
	std::array<int16_t, (int)CddaFrame::Audio/2> buffer;
	HardwareInfo hwinfo;
	Toc toc;
	std::map<int, std::string> cdText;

private:
	int m_nrSectors;
	data_fcn_t m_data;
};


TEST_CASE("SectorDb")
{
	std::array<int, 3> nrReads{0};
	SectorDb sdb((int)nrReads.size());
	SectorDb::Statistics stat;

	auto trueData = [](int lba)
	{
		return lba + 1;
	};

	SECTION("Read once")
	{
		auto readNoError = [&](int lba)
		{
			nrReads[lba]++;
			return trueData(lba);
		};
		MockReader reader((int)nrReads.size(), readNoError);
		int minCrcCount = 1;

		stat = sdb.GetStatistics();
		CHECK(stat.minMC == 0);
		CHECK(stat.maxMC == 0);

		while(sdb.Refine(reader, minCrcCount)) {}
		stat = sdb.GetStatistics();
		CHECK(stat.minMC == minCrcCount);
		CHECK(stat.maxMC == minCrcCount);

		for(int lba=0; lba < (int)nrReads.size(); lba++)
			CHECK(Audio(sdb[lba])[0] == trueData(lba));
	}

	SECTION("Read twice")
	{
		auto readNoError = [&](int lba)
		{
			nrReads[lba]++;
			return trueData(lba);
		};
		MockReader reader((int)nrReads.size(), readNoError);
		int minCrcCount = 2;

		while(sdb.Refine(reader, minCrcCount)) {}
		stat = sdb.GetStatistics();
		CHECK(stat.minMC == minCrcCount);
		CHECK(stat.maxMC == minCrcCount);

		for(int lba=0; lba < (int)nrReads.size(); lba++)
			CHECK(Audio(sdb[lba])[0] == trueData(lba));
	}

	SECTION("Read one error")
	{
		std::vector<int> errorSequence = {1, 0, 0};
		std::vector<bool> sectorHasError = {false, true, true};

		auto readOneError = [&](int lba)
		{
			auto iErr = nrReads[lba];
			nrReads[lba]++;
			uint8_t r = trueData(lba);
			if(sectorHasError[lba])
				r += errorSequence.at(iErr);
			return r;
		};

		MockReader reader((int)nrReads.size(), readOneError);
		int minCrcCount = 2;

		// 1st read
		int nrRead1 = sdb.Refine(reader, minCrcCount);
		CHECK(nrRead1 == 3);
		stat = sdb.GetStatistics();
		CHECK(stat.minMC == 1);
		CHECK(stat.maxMC == 1);

		// 2nd read
		int nrRead2 = sdb.Refine(reader, minCrcCount);
		CHECK(nrRead2 == 3);
		stat = sdb.GetStatistics();
		CHECK(stat.minMC == 1);
		CHECK(stat.maxMC == 2);

		// 3rd read
		int nrRead3 = sdb.Refine(reader, minCrcCount);
		CHECK(nrRead3 == 2);
		stat = sdb.GetStatistics();
		CHECK(stat.minMC == 2);
		CHECK(stat.maxMC == 2);

		// Data has to match, with minimum amount of reads
		for(int lba=0; lba < (int)nrReads.size(); lba++)
		{
			CHECK(Audio(sdb[lba])[0] == trueData(lba));
			int expectedReads = sectorHasError[lba] ? 3 : 2;
			CHECK(nrReads[lba] == expectedReads);
		}
	}
} // Test case: Sector DB
