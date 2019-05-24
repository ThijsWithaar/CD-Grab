#include <cdgrab/file/flac.h>

#include <iostream>

#include <boost/format.hpp>


namespace detail {

void progress_callback(
	const FLAC__StreamEncoder *encoder,
	FLAC__uint64 bytes_written,
	FLAC__uint64 samples_written,
	unsigned frames_written,
	unsigned total_frames_estimate,
	void *client_data)
{
	FlacStreamEncoder::Progress p;
	p.bytesWritten = bytes_written;
	p.samplesWritten = samples_written;
	//std::cout << boost::format("Flac Progress: %x\n") % client_data;

	//auto self = reinterpret_cast<FlacStreamEncoder*>(client_data);
	//self->progress(frames_written, total_frames_estimate);
	auto cb = reinterpret_cast<FlacStreamEncoder::Callback*>(client_data);
	if(*cb)
		(*cb)(p);
}


} // detail


class FlacResult
{
public:
	FlacResult(bool r = true):
		mr(r)
	{
		if(!mr)
			throw std::runtime_error("Flac error");
	}

	~FlacResult()
	{
	}

private:
	FLAC__bool mr;
};


// Default, non optimized
void LoadBuffer(std::vector<FLAC__int32>& dst, buffer_view<int16_t> src)
{
	dst.resize(src.size());
	for(int n=0; n < src.size(); n++)
		dst[n] = (src.data()[n]);
}


MetaData::MetaData(FLAC__MetadataType type):
	m(FLAC__metadata_object_new(type))
{
}


MetaData::~MetaData()
{
	FLAC__metadata_object_delete(m);
}


FLAC__StreamMetadata& MetaData::operator*()
{
	return *m;
}



FlacStreamEncoder::FlacStreamEncoder(std::string fname, int nrSamples, Callback progress):
	m_encoder(FLAC__stream_encoder_new()),
	m_progress(progress)
{
	FlacResult r;
	r = FLAC__stream_encoder_set_channels(m_encoder, 2);
	r = FLAC__stream_encoder_set_bits_per_sample(m_encoder, 16);
	r = FLAC__stream_encoder_set_sample_rate(m_encoder, 44100);
	r = FLAC__stream_encoder_set_compression_level(m_encoder, 8);
	r = FLAC__stream_encoder_set_total_samples_estimate(m_encoder, nrSamples);

	if(auto s = FLAC__stream_encoder_init_file(m_encoder, fname.c_str(), detail::progress_callback, &m_progress) != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
	{
		throw std::runtime_error("Flac Encoder init error" + std::to_string(s));
	}
	//FLAC__stream_encoder_init_file(m_encoder, fname.c_str(), nullptr, nullptr);
}


FlacStreamEncoder::~FlacStreamEncoder()
{
	FLAC__stream_encoder_finish(m_encoder);	// Not required
	FLAC__stream_encoder_delete(m_encoder);
}


void FlacStreamEncoder::SetInfo()
{
	// Need to keep the objects until after encoding finishes...
	m_meta.push_back(FLAC__METADATA_TYPE_VORBIS_COMMENT);
	m_meta.push_back(FLAC__METADATA_TYPE_PADDING);

	std::vector<FLAC__StreamMetadata*> pMeta;
	for(auto& m: m_meta)
		pMeta.push_back(&(*m));
	FlacResult r = FLAC__stream_encoder_set_metadata(m_encoder, pMeta.data(), (unsigned)pMeta.size());
}


void FlacStreamEncoder::Append(buffer_view<int16_t> bview)
{
	LoadBuffer(m_samples, bview);
	const unsigned samples_per_channel = (unsigned)m_samples.size()/2;
	const FLAC__int32* buffer = m_samples.data();
	if(!FLAC__stream_encoder_process_interleaved(m_encoder, buffer, samples_per_channel))
		throw std::runtime_error("Flac encoding error");
}
