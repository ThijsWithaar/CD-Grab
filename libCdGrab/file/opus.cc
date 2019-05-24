#include <cdgrab/file/opus.h>

#include <cassert>


namespace CdGrab {


#if defined(OPUS_USE_SOXR)


Resampler::Resampler(int nrChannels)
{
	soxr_error_t err = 0;
	soxr_io_spec_t io_spec = soxr_io_spec(SOXR_INT16_I, SOXR_FLOAT32);
	//m_soxr = soxr_create(m_src_rate, m_dst_rate, m_nr_channels, &err, &io_spec, nullptr, nullptr);
	if(err != 0)
		throw std::runtime_error(soxr_strerror(err));
}


Resampler::~Resampler()
{
	soxr_delete(m_soxr);
}


buffer_view<float> Resampler::operator()(buffer_view<uint16_t> in, bool flush)
{
	m_resampled.resize(2*in.size());

	size_t idone = 0, odone = 0;
	soxr_process(m_soxr,
		in.data(), in.size()/2, &idone,
		m_resampled.data(), m_resampled.size()/2, &odone);

	// Keep copy of non-processed samples
	assert(in.size()/2 == idone);

	return{m_resampled.data(), (int)odone};
}


#elif defined(OPUS_USE_SAMPLERATE)


Resampler::Resampler(int nrChannels)
{
	int error;
	m_src = src_new(SRC_SINC_BEST_QUALITY, nrChannels, &error);
	if(error != 0)
		throw std::runtime_error(src_strerror(error));
	src_set_ratio (m_src, double(m_src_rate)/m_dst_rate) ;
}


Resampler::~Resampler()
{
	src_delete(m_src);
}


buffer_view<float> Resampler::operator()(buffer_view<uint16_t> in, bool flush)
{
	// See: http://www.mega-nerd.com/libsamplerate/api_full.html
	m_recoded.resize(in.size());
	m_resampled.resize(2*in.size());

	src_short_to_float_array(reinterpret_cast<const short*>(in.data()), m_recoded.data(), in.size());

	SRC_DATA data;
	data.data_in = m_recoded.data();
	data.input_frames = m_recoded.size() / 2;
	data.data_out = m_resampled.data();
	data.output_frames = m_resampled.size() / 2;
	data.end_of_input = flush ? 1 : 0;
	data.src_ratio = double(m_src_rate)/m_dst_rate;
	src_process(m_src, &data);

	return buffer_view<float>(m_resampled.data(), data.output_frames_gen);
}
#endif


//-- OpusEncoder --


OpusEncoder::OpusEncoder(const std::string& fname):
	m_encoder(nullptr),
	m_resampler(2),
	m_file(fname, std::ios::binary),
	m_encoded(0)
{
	const int nrChannels = 2;
	int error;
	m_encoder = opus_encoder_create(48000, nrChannels, OPUS_APPLICATION_AUDIO, &error);
	opus_encoder_ctl(m_encoder, OPUS_SET_COMPLEXITY(10));	// range 0 .. 10
	opus_encoder_ctl(m_encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC));
	opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(96*1000));
	opus_encoder_ctl(m_encoder, OPUS_SET_LSB_DEPTH(16));
}


OpusEncoder::~OpusEncoder()
{
	opus_encoder_destroy(m_encoder);
}


void OpusEncoder::Append(buffer_view<uint16_t> bview, bool flush)
{
	// Resample to 48KHz
	buffer_view<float> pcm48Khz = m_resampler(bview, flush);

	m_encoded.resize(bview.size());
	buffer_view<uint8_t> encoded{m_encoded.data(), (int)m_encoded.size()};

	int nrBytesEncoded = opus_encode_float(m_encoder,
										   pcm48Khz.data(),
										   (opus_int32)pcm48Khz.size(),
										   encoded.data(),
										   (opus_int32)encoded.size());

	m_file.write((const char*)encoded.data(), nrBytesEncoded);
}


} // namespace CdGrab
