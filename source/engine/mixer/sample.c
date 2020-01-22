/*-----------------------------

MIT License

Copyright (c) 2019 Alexander Brandt

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

-------------------------------

 [sample.c]
 - Alexander Brandt 2019-2020
-----------------------------*/

#include "private.h"


static void sToCommonFormat(const void* in, size_t in_size, size_t in_channels, enum jaSoundFormat in_format, float* out,
                            size_t out_channels)
{
	union {
		const void* raw;
		int8_t* i8;
		int16_t* i16;
		int32_t* i32;
		float* f32;
		double* f64;
	} org;

	org.raw = in;
	float mix;

	// Cycle in frame steps
	for (size_t bytes_read = 0; bytes_read < in_size; bytes_read += ((size_t)jaBytesPerSample(in_format) * in_channels))
	{
		// Re format (now cycling though samples)
		for (size_t c = 0; c < in_channels; c++)
		{
			switch (in_format)
			{
			case SOUND_I8: mix = ((float)org.i8[c]) / (float)INT8_MAX; break;
			case SOUND_I16: mix = ((float)org.i16[c]) / (float)INT16_MAX; break;
			case SOUND_I32: mix = ((float)org.i32[c]) / (float)INT32_MAX; break;
			case SOUND_F32: mix = org.f32[c]; break;
			case SOUND_F64: mix = (float)org.f64[c];
			}

			// Mix, this only works with stereo and mono.
			// Different files can map channels in their own way.

			// TODO: This map thing should be abstracted in LibJapan
			out[c % out_channels] += mix;

			if (c >= out_channels)
				out[c % out_channels] /= 2.0f;
		}

		// Next
		org.i8 += ((size_t)jaBytesPerSample(in_format) * in_channels);
		out += out_channels;
	}
}


/*-----------------------------

 SampleCreate()
-----------------------------*/
struct Sample* SampleCreate(struct Mixer* mixer, const char* filename, struct jaStatus* st)
{
	struct jaDictionaryItem* item = NULL;
	struct Sample* sample = NULL;
	struct jaSoundEx ex = {0};
	FILE* file = NULL;

	jaStatusSet(st, "SampleCreate", STATUS_SUCCESS, NULL);
	FreeMarkedSamples(mixer);

	// Alredy exists?
	item = jaDictionaryGet(mixer->samples, filename);

	if (item != NULL)
		return item->data;

	// Open
	if ((file = fopen(filename, "rb")) == NULL)
	{
		jaStatusSet(st, "SampleCreate", STATUS_IO_ERROR, "File: \"%s\"", filename);
		return NULL;
	}

	if (jaSoundExLoad(file, &ex, st) != 0)
		goto return_failure;

	if (fseek(file, (long)ex.data_offset, SEEK_SET) != 0)
	{
		jaStatusSet(st, "SampleCreate", STATUS_UNEXPECTED_EOF, "File: \"%s\"", filename);
		goto return_failure;
	}

	if (ex.uncompressed_size > (10 * 1024 * 1024))
	{
		jaStatusSet(st, "SampleCreate", STATUS_UNSUPPORTED_FEATURE, "Only files small than 10 Mb supported. File: \"%s\"", filename);
		goto return_failure;
	}

	// Reformat (we use float everywhere)
	float* reformat_dest = NULL;
	{
		size_t reformat_size = ex.length * jaMin(ex.channels, (size_t)mixer->cfg.channels) * sizeof(float);
		struct jaStatus temp_st = {0};

		if (jaBufferResize(&mixer->buffer, ex.uncompressed_size + reformat_size) == NULL)
		{
			jaStatusSet(st, "SampleCreate", STATUS_MEMORY_ERROR, "File: \"%s\"", filename);
			goto return_failure;
		}

		reformat_dest = (float*)(((uint8_t*)mixer->buffer.data) + ex.uncompressed_size);
		memset(reformat_dest, 0, reformat_size);

		jaSoundExRead(file, ex, ex.uncompressed_size, mixer->buffer.data, &temp_st);

		if (temp_st.code != STATUS_SUCCESS)
		{
			if (st != NULL)
				memcpy(st, &temp_st, sizeof(struct jaStatus));

			goto return_failure;
		}

		sToCommonFormat(mixer->buffer.data, ex.uncompressed_size, ex.channels, ex.format, reformat_dest,
		                jaMin(ex.channels, (size_t)mixer->cfg.channels));
	}

	// Resample
	{
		size_t resampled_length = ex.length * (size_t)ceil((double)mixer->cfg.frequency / (double)ex.frequency);
		size_t resampled_size = resampled_length * sizeof(float) * jaMin(ex.channels, (size_t)mixer->cfg.channels);

		if ((item = jaDictionaryAdd(mixer->samples, filename, NULL, sizeof(struct Sample) + resampled_size)) == NULL)
		{
			jaStatusSet(st, "SampleCreate", STATUS_MEMORY_ERROR, "File: \"%s\"", filename);
			goto return_failure;
		}

		sample = item->data;
		sample->item = item;
		sample->length = resampled_length;
		sample->channels = jaMin(ex.channels, (size_t)mixer->cfg.channels);
		sample->references = 0;
		sample->to_delete = false;

		memset(sample->data, 0, resampled_size);

		SRC_DATA resample_cfg = {.data_in = reformat_dest,
		                         .data_out = sample->data,
		                         .input_frames = (long)ex.length,
		                         .output_frames = (long)resampled_length,
		                         .src_ratio = (double)mixer->cfg.frequency / (double)ex.frequency};

		if (src_simple(&resample_cfg, mixer->cfg.sampling, jaMin((int)ex.channels, mixer->cfg.channels)) != 0)
		{
			jaStatusSet(st, "SampleCreate", STATUS_ERROR, "src_simple() error. File: \"%s\"", filename);
			goto return_failure;
		}
	}

	// Resize list to keep deleted samples
	mixer->samples_no += 1;

	if (jaBufferResizeZero(&mixer->marked_samples, sizeof(void*) * mixer->samples_no) == NULL)
	{
		jaStatusSet(st, "SampleCreate", STATUS_MEMORY_ERROR, "File: \"%s\"", filename);
		goto return_failure;
	}

	// Bye!
	fclose(file);
	return sample;

return_failure:
	if (file != NULL)
		fclose(file);
	if (item != NULL)
		jaDictionaryRemove(item);

	return NULL;
}


/*-----------------------------

 SampleDelete()
-----------------------------*/
inline void SampleDelete(struct Sample* sample)
{
	sample->to_delete = true;
}
