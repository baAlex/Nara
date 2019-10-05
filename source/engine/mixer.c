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

 [mixer.c]
 - Alexander Brandt 2019
-----------------------------*/

#include <math.h>
#include <portaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "buffer.h"
#include "dictionary.h"
#include "mixer.h"
#include "samplerate.h"
#include "utilities.h"

#define PLAY_LEN 8


struct Sample
{
	struct DictionaryItem* item;

	size_t length;
	size_t channels;
	float data[];
};

struct ToPlay
{
	bool active;
	size_t cursor;

	struct Sample* sample;
	float volume;
};

struct Mixer
{
	PaStream* stream;
	struct MixerOptions options;
	struct Dictionary* samples;
	struct Buffer buffer;

	struct ToPlay playlist[PLAY_LEN];
	struct timespec playlist_update;

	size_t last_index;
};


/*-----------------------------

 sCallback
-----------------------------*/
static int sCallback(const void* raw_input, void* raw_output, unsigned long frames_no,
                     const PaStreamCallbackTimeInfo* time, PaStreamCallbackFlags status, void* raw_mixer)
{
	(void)raw_input;
	(void)time;
	(void)status;

	struct Mixer* mixer = raw_mixer;
	float* output = raw_output;
	float* data = NULL;

	const size_t channels = (size_t)mixer->options.channels;

	for (unsigned long i = 0; i < (frames_no * channels); i += channels)
	{
		for (size_t ch = 0; ch < channels; ch++)
			output[i + ch] = 0.0f;

		// Sum all playlist
		for (size_t pl = 0; pl < PLAY_LEN; pl++)
		{
			if (mixer->playlist[pl].active == false)
				continue;

			data = mixer->playlist[pl].sample->data;
			data += (mixer->playlist[pl].cursor * mixer->playlist[pl].sample->channels);

			for (size_t ch = 0; ch < channels; ch++)
				output[i + ch] += (data[ch % mixer->playlist[pl].sample->channels] * mixer->playlist[pl].volume);

			if ((mixer->playlist[pl].cursor += 1) > mixer->playlist[pl].sample->length)
				mixer->playlist[pl].active = false;
		}

		// Global volume
		for (size_t ch = 0; ch < channels; ch++)
		{
			output[i + ch] *= mixer->options.volume;

			if (output[i + ch] > 1.0f)
			{
				// TODO, peak limiter
			}
		}
	}

	return paContinue;
}


/*-----------------------------

 sToCommonFormat
-----------------------------*/
static void sToCommonFormat(const void* in, size_t in_size, size_t in_channels, enum SoundFormat in_format, float* out,
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
	for (size_t bytes_read = 0; bytes_read < in_size; bytes_read += ((size_t)SoundBps(in_format) * in_channels))
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
			out[c % out_channels] += mix;

			if (c >= out_channels)
				out[c % out_channels] /= 2.0f;
		}

		// Next
		org.i8 += ((size_t)SoundBps(in_format) * in_channels);
		out += out_channels;
	}
}


/*-----------------------------

 MixerCreate()
-----------------------------*/
struct Mixer* MixerCreate(struct MixerOptions options, struct Status* st)
{
	struct Mixer* mixer = NULL;

	StatusSet(st, "MixerCreate", STATUS_SUCCESS, NULL);
	printf("- Lib-Portaudio: %s\n", (Pa_GetVersionInfo() != NULL) ? Pa_GetVersionInfo()->versionText : "");

	// Initialization
	if ((mixer = calloc(1, sizeof(struct Mixer))) == NULL)
		return NULL;

	if ((mixer->samples = DictionaryCreate(NULL)) == NULL)
		goto return_failure;

	options.channels = Clamp(options.channels, 1, 2);
	options.frequency = Clamp(options.frequency, 6000, 48000);
	options.volume = Clamp(options.volume, 0.0f, 1.0f);

	memcpy(&mixer->options, &options, sizeof(struct MixerOptions));

	if (Pa_Initialize() != paNoError)
	{
		StatusSet(st, "MixerCreate", STATUS_ERROR, "Initialiting Portaudio");
		goto return_failure;
	}

	// Create stream
	if (Pa_OpenDefaultStream(&mixer->stream, 0, options.channels, paFloat32, (double)options.frequency,
	                         paFramesPerBufferUnspecified, sCallback, mixer) != paNoError)
	{
		StatusSet(st, "MixerCreate", STATUS_ERROR, "Opening stream");
		goto return_failure;
	}

	Pa_StartStream(mixer->stream);

	// Bye!
	printf("\n");
	return mixer;

return_failure:
	MixerDelete(mixer);
	return NULL;
}


/*-----------------------------

 MixerDelete()
-----------------------------*/
inline void MixerDelete(struct Mixer* mixer)
{
	if (mixer->stream != NULL)
		Pa_StopStream(mixer->stream);

	Pa_Terminate();

	DictionaryDelete(mixer->samples);
	BufferClean(&mixer->buffer);
	free(mixer);
}


/*-----------------------------

 SampleCreate()
-----------------------------*/
struct Sample* SampleCreate(struct Mixer* mixer, const char* filename, struct Status* st)
{
	struct DictionaryItem* item = NULL;
	struct Sample* sample = NULL;
	struct SoundEx ex = {0};
	FILE* file = NULL;

	StatusSet(st, "SampleCreate", STATUS_SUCCESS, NULL);

	// Alredy exists?
	item = DictionaryGet(mixer->samples, filename);

	if (item != NULL)
		return item->data;

	// Open
	if ((file = fopen(filename, "rb")) == NULL)
	{
		StatusSet(st, "SampleCreate", STATUS_IO_ERROR, NULL);
		return NULL;
	}

	if (SoundExLoad(file, &ex, st) != 0)
		goto return_failure;

	if (fseek(file, (long)ex.data_offset, SEEK_SET) != 0)
	{
		StatusSet(st, "SampleCreate", STATUS_UNEXPECTED_EOF, "at data seek");
		goto return_failure;
	}

	if (ex.uncompressed_size > (10 * 1024 * 1024))
	{
		StatusSet(st, "SampleCreate", STATUS_UNSUPPORTED_FEATURE,
		          "The dev is lazy, only files small than 10mb supported");
		goto return_failure;
	}

	// Reformat (we use float everywhere)
	float* reformat_dest = NULL;
	{
		size_t reformat_size = ex.length * Min(ex.channels, (size_t)mixer->options.channels) * sizeof(float);
		struct Status temp_st = {0};

		if (BufferResize(&mixer->buffer, ex.uncompressed_size + reformat_size) == NULL)
		{
			StatusSet(st, "SampleCreate", STATUS_MEMORY_ERROR, NULL);
			goto return_failure;
		}

		reformat_dest = (float*)(((uint8_t*)mixer->buffer.data) + ex.uncompressed_size);
		memset(reformat_dest, 0, reformat_size);

		SoundExRead(file, ex, ex.uncompressed_size, mixer->buffer.data, &temp_st);

		if (temp_st.code != STATUS_SUCCESS)
			goto return_failure;

		sToCommonFormat(mixer->buffer.data, ex.uncompressed_size, ex.channels, ex.format, reformat_dest,
		                Min(ex.channels, (size_t)mixer->options.channels));
	}

	// Resample
	{
		size_t resampled_length = ex.length * (size_t)mixer->options.frequency / ex.frequency;
		size_t resampled_size = resampled_length * sizeof(float) * Min(ex.channels, (size_t)mixer->options.channels);

		if ((item = DictionaryAdd(mixer->samples, filename, NULL, sizeof(struct Sample) + resampled_size)) == NULL)
		{
			StatusSet(st, "SampleCreate", STATUS_MEMORY_ERROR, NULL);
			goto return_failure;
		}

		sample = item->data;
		sample->item = item;
		sample->length = resampled_length;
		sample->channels = Min(ex.channels, (size_t)mixer->options.channels);

		SRC_DATA resample_cfg = {
			.data_in = reformat_dest,
			.data_out = sample->data,
			.input_frames = (long)ex.length,
			.output_frames = (long)resampled_length,
			.src_ratio = (double)mixer->options.frequency / (double)ex.frequency
		};

		// TODO, hardcoded SRC_LINEAR
		if (src_simple(&resample_cfg, SRC_LINEAR, Min((int)ex.channels, mixer->options.channels)) != 0)
		{
			StatusSet(st, "SampleCreate", STATUS_ERROR, "src_simple() error");
			goto return_failure;
		}
	}

	// Bye!
	fclose(file);
	return sample;

return_failure:
	if (file != NULL)
		fclose(file);
	if (item != NULL)
		DictionaryRemove(item);

	return NULL;
}


/*-----------------------------

 PlaySample()
-----------------------------*/
inline void PlayFile(struct Mixer* mixer, float volume, const char* filename)
{
	struct DictionaryItem* item = DictionaryGet(mixer->samples, filename);
	struct Sample* sample = NULL;

	if (item != NULL)
		PlaySample(mixer, volume, (struct Sample*)item->data);
	else
	{
		printf("Creating sample for '%s'...\n", filename);

		if ((sample = SampleCreate(mixer, filename, NULL)) != NULL)
			PlaySample(mixer, volume, sample);
	}
}

void PlaySample(struct Mixer* mixer, float volume, struct Sample* sample)
{
	struct ToPlay* space = NULL;

	// Find an space in the playlist
	{
		size_t ref_i = 0;
		size_t i = 0;

		for (ref_i = 0; ref_i < PLAY_LEN; ref_i++)
		{
			i = (mixer->last_index + ref_i) % PLAY_LEN;

			if (mixer->playlist[i].active == false)
			{
				space = &mixer->playlist[i];
				break;
			}
		}

		if (ref_i == PLAY_LEN) // Bad luck, lets recycle
		{
			i = (mixer->last_index + 1) % PLAY_LEN;
			space = &mixer->playlist[i];
		}

		// printf("Play at %zu%s\n", i, (ref_i == PLAY_LEN) ? " (r)" : "");
		mixer->last_index = (i + 1);
	}

	// Yay
	space->active = false;

	space->cursor = 0;
	space->sample = sample;
	space->volume = volume;

	space->active = true;
}
