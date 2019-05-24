#pragma once

#include <functional>

#include <FLAC/stream_encoder.h>
#include <FLAC/metadata.h>

#include "../buffer_view.h"
#include "../grab_export.h"


class MetaData
{
public:
	MetaData(FLAC__MetadataType type);

	~MetaData();

	FLAC__StreamMetadata& operator*();

private:
	FLAC__StreamMetadata* m;
};


class GRAB_EXPORT FlacStreamEncoder
{
public:
	struct Progress
	{
		uint64_t samplesWritten;
		uint64_t bytesWritten;
	};

	typedef std::function<void(Progress)> Callback;

	FlacStreamEncoder(std::string fname, int nrSamples, Callback progress=Callback());

	FlacStreamEncoder(const FlacStreamEncoder&) = delete;

	~FlacStreamEncoder();

	void SetInfo();

	void Append(buffer_view<int16_t> bview);

private:
	std::vector<FLAC__int32> m_samples;

	FLAC__StreamEncoder *m_encoder;
	std::vector<MetaData> m_meta;
	Callback m_progress;
};
