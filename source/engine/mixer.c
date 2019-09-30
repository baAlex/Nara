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

#include "dictionary.h"
#include "mixer.h"
#include "utilities.h"

#define PLAY_LEN 8


struct Sample
{
	struct DictionaryItem* item;

	size_t length;
	float data[];
};

struct ToPlay
{
	bool active;
	bool is_synth;

	size_t cursor;

	union {
		struct Sample* sample;

		struct
		{
			size_t length;
			float frequency;
		} synth;
	};

	float volume;
};

struct Mixer
{
	PaStream* stream;
	struct MixerOptions options;
	struct Dictionary* samples;

	struct ToPlay playlist[PLAY_LEN];
	struct timespec playlist_update;

	size_t last_index;
};


static inline float sSaw(float cur, float frequency, float base_frequency)
{
	// return sinf(cur * (2.0f * M_PI * frequency) / base_frequency);
	return (fmodf(cur * frequency * 2.0f / base_frequency + 1.0f, 2.0f) - 1.0f);
}


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

			if (mixer->playlist[pl].is_synth == false)
			{
				data = mixer->playlist[pl].sample->data;
				data += (mixer->playlist[pl].cursor * channels);

				for (size_t ch = 0; ch < channels; ch++)
					output[i + ch] += (data[ch] * mixer->playlist[pl].volume);

				if ((mixer->playlist[pl].cursor += 1) > mixer->playlist[pl].sample->length)
					mixer->playlist[pl].active = false;
			}
			else
			{
				for (size_t ch = 0; ch < channels; ch++)
					output[i + ch] += sSaw((float)mixer->playlist[pl].cursor, mixer->playlist[pl].synth.frequency,
					                       (float)mixer->options.frequency) *
					                  mixer->playlist[pl].volume;

				if ((mixer->playlist[pl].cursor += 1) > mixer->playlist[pl].synth.length)
					mixer->playlist[pl].active = false;
			}
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

	// Well, lets allocate everything needed
	size_t total_size = sizeof(struct Sample) + (sizeof(float) * (size_t)mixer->options.channels);

	if ((file = fopen(filename, "rb")) == NULL)
	{
		StatusSet(st, "SampleCreate", STATUS_IO_ERROR, NULL);
		return NULL;
	}

	if (SoundExLoad(file, &ex, st) != 0)
		goto return_failure;

	if ((item = DictionaryAdd(mixer->samples, filename, NULL, total_size)) == NULL)
	{
		StatusSet(st, "SampleCreate", STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	sample = item->data;
	sample->item = item;

	sample->length = 1;

	// Resampling
	{
	}

	// Bye!
	fclose(file);
	return sample;

return_failure:
	if (file != NULL)
		fclose(file);

	return NULL;
}


/*-----------------------------

 PlayTone()
-----------------------------*/
void PlayTone(struct Mixer* mixer, float volume, float frequency, int duration)
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

		printf("Play at %zu%s\n", i, (ref_i == PLAY_LEN) ? " (r)" : "");
		mixer->last_index = (i + 1);
	}

	// Yay
	space->active = false;

	space->cursor = 0;
	space->sample = NULL;
	space->is_synth = true;

	space->volume = volume;
	space->synth.length = ((size_t)duration * (size_t)mixer->options.frequency) / 1000;
	space->synth.frequency = frequency;

	space->active = true;
}
