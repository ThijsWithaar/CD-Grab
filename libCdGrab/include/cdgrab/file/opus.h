#pragma once

#include <fstream>

#include <opus/opus.h>

#if defined(OPUS_USE_SOXR)
	#include <soxr.h>
#elif defined(OPUS_USE_SAMPLERATE)
	#include <samplerate.h>
#endif

#include "../buffer_view.h"


namespace CdGrab {


class Resampler
{
public:
	Resampler(int nrChannels);

	~Resampler();

	buffer_view<float> operator()(buffer_view<uint16_t> in, bool flush);

private:
	const static int m_nr_channels = 2;
	const static int m_src_rate = 44100;
	const static int m_dst_rate = 48000;
#if defined(OPUS_USE_SOXR)
	std::vector<float> m_resampled;
	soxr_t m_soxr;
#elif defined(OPUS_USE_SAMPLERATE)
	std::vector<float> m_recoded, m_resampled;
	SRC_STATE* m_src;
#endif
};


class OpusEncoder
{
public:
	OpusEncoder(const std::string& fname);

	OpusEncoder(const OpusEncoder&) = delete;

	~OpusEncoder();

	OpusEncoder& operator=(const OpusEncoder&) = delete;

	void Append(buffer_view<uint16_t> bview, bool flush);

private:
	::OpusEncoder* m_encoder;
	Resampler m_resampler;
	std::ofstream m_file;
	std::vector<uint8_t> m_encoded;
};


} // namespace CdGrab
