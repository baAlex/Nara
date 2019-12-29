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

#include "japan-buffer.h"
#include "japan-dictionary.h"
#include "japan-utilities.h"

#include "mixer.h"
#include "samplerate.h"

#define PLAY_LEN 8


struct Sample
{
	struct jaDictionaryItem* item;

	int references;
	bool to_delete;

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

	enum PlayOptions options;
};

struct Mixer
{
	PaStream* stream;
	struct jaBuffer buffer;
	struct
	{
		int frequency;
		int channels;
		float volume;
	} cfg;

	struct jaDictionary* samples;
	size_t samples_no;
	struct jaBuffer marked_samples; // To free them

	struct ToPlay playlist[PLAY_LEN];
	size_t last_index;
};


/*-----------------------------

 sFreeMarkedSamples()
-----------------------------*/
static void sFreeMarkedSamples(struct Mixer* mixer)
{
	for (size_t i = 0; i < mixer->samples_no; i++)
	{
		if (((struct jaDictionaryItem**)mixer->marked_samples.data)[i] != NULL)
			jaDictionaryRemove(((struct jaDictionaryItem**)mixer->marked_samples.data)[i]);
	}
}


/*-----------------------------

 sCallback()
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

	const size_t channels = (size_t)mixer->cfg.channels;

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

			if ((mixer->playlist[pl].cursor += 1) >= mixer->playlist[pl].sample->length)
			{
				if ((mixer->playlist[pl].options & PLAY_LOOP) == PLAY_LOOP)
				{
					mixer->playlist[pl].cursor = 0;
					continue;
				}

				mixer->playlist[pl].active = false;
				mixer->playlist[pl].sample->references -= 1;

				// Mark sample as deleted if there are no further use
				if (mixer->playlist[pl].sample->to_delete == true && mixer->playlist[pl].sample->references <= 0)
				{
					struct jaDictionaryItem* item = mixer->playlist[pl].sample->item;
					jaDictionaryDetach(item);

					for (size_t z = 0; z < mixer->samples_no; z++)
					{
						if (((struct jaDictionaryItem**)mixer->marked_samples.data)[z] == NULL)
							((struct jaDictionaryItem**)mixer->marked_samples.data)[z] = item;
					}
				}
			}
		}

		// Global volume
		for (size_t ch = 0; ch < channels; ch++)
		{
			output[i + ch] *= mixer->cfg.volume;

			if (output[i + ch] > 1.0f)
			{
				// TODO, peak limiter
			}
		}
	}

	return paContinue;
}


/*-----------------------------

 sToCommonFormat()
-----------------------------*/
static void sToCommonFormat(const void* in, size_t in_size, size_t in_channels, enum jaSoundFormat in_format,
                            float* out, size_t out_channels)
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

 MixerCreate()
-----------------------------*/
struct Mixer* MixerCreate(const struct jaConfig* config, struct jaStatus* st)
{
	struct Mixer* mixer = NULL;
	PaError errcode = paNoError;

	jaStatusSet(st, "MixerCreate", STATUS_SUCCESS, NULL);
	printf("- Lib-Portaudio: %s\n", (Pa_GetVersionInfo() != NULL) ? Pa_GetVersionInfo()->versionText : "");

	// Initialization
	if ((mixer = calloc(1, sizeof(struct Mixer))) == NULL || (mixer->samples = jaDictionaryCreate(NULL)) == NULL)
	{
		jaStatusSet(st, "MixerCreate", STATUS_MEMORY_ERROR, NULL);
		return NULL;
	}

	if (jaConfigRetrieve(config, "sound.volume", &mixer->cfg.volume, st) != 0 ||
	    jaConfigRetrieve(config, "sound.frequency", &mixer->cfg.frequency, st) != 0 ||
	    jaConfigRetrieve(config, "sound.channels", &mixer->cfg.channels, st) != 0)
		goto return_failure;

	if ((errcode = Pa_Initialize()) != paNoError)
	{
		jaStatusSet(st, "MixerCreate", STATUS_ERROR, "Initialiting Portaudio: \"%s\"", Pa_GetErrorText(errcode));
		goto return_failure;
	}

	// Create stream
	if ((errcode = Pa_OpenDefaultStream(&mixer->stream, 0, mixer->cfg.channels, paFloat32, (double)mixer->cfg.frequency,
	                                    paFramesPerBufferUnspecified, sCallback, mixer)) != paNoError)
	{
		jaStatusSet(st, "MixerCreate", STATUS_ERROR, "Opening stream: \"%s\"", Pa_GetErrorText(errcode));
		goto return_failure;
	}

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
	if (mixer != NULL)
	{
		if (mixer->stream != NULL)
			Pa_StopStream(mixer->stream);

		Pa_Terminate();

		jaDictionaryDelete(mixer->samples);
		jaBufferClean(&mixer->buffer);
		sFreeMarkedSamples(mixer);
		free(mixer);
	}
}


/*-----------------------------

 MixerStart()
-----------------------------*/
int MixerStart(struct Mixer* mixer, struct jaStatus* st)
{
	PaError errcode = paNoError;

	if ((errcode = Pa_StartStream(mixer->stream)) != paNoError)
	{
		jaStatusSet(st, "MixerCreate", STATUS_ERROR, "Starting stream: \"%s\"", Pa_GetErrorText(errcode));
		return 1;
	}

	return 0;
}


/*-----------------------------

 MixerStop()
-----------------------------*/
void MixerStop(struct Mixer* mixer)
{
	(void)mixer;
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
	sFreeMarkedSamples(mixer);

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
		jaStatusSet(st, "SampleCreate", STATUS_UNSUPPORTED_FEATURE,
		            "Only files small than 10 Mb supported. File: \"%s\"", filename);
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

		// TODO, hardcoded SRC_LINEAR
		if (src_simple(&resample_cfg, SRC_LINEAR, jaMin((int)ex.channels, mixer->cfg.channels)) != 0)
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


/*-----------------------------

 Play2d
-----------------------------*/
inline void Play2dFile(struct Mixer* mixer, float volume, enum PlayOptions options, const char* filename)
{
	struct jaDictionaryItem* item = jaDictionaryGet(mixer->samples, filename);
	struct Sample* sample = NULL;

	if (item != NULL)
		Play2dSample(mixer, volume, options, (struct Sample*)item->data);
	else
	{
		printf("Creating sample for '%s'...\n", filename);

		if ((sample = SampleCreate(mixer, filename, NULL)) != NULL)
			Play2dSample(mixer, volume, options, sample);
	}
}

void Play2dSample(struct Mixer* mixer, float volume, enum PlayOptions options, struct Sample* sample)
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
	sample->references += 1;

	space->active = false;

	space->cursor = 0;
	space->options = options;
	space->sample = sample;
	space->volume = volume;

	space->active = true;
}
