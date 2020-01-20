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
 - Alexander Brandt 2019-2020
-----------------------------*/

#include "private.h"


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

 MixerCreate()
-----------------------------*/
struct Mixer* MixerCreate(const struct jaConfiguration* config, struct jaStatus* st)
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

	if (jaCvarRetrieve(config, "sound.volume", &mixer->cfg.volume, st) != 0 ||
	    jaCvarRetrieve(config, "sound.frequency", &mixer->cfg.frequency, st) != 0 ||
	    jaCvarRetrieve(config, "sound.channels", &mixer->cfg.channels, st) != 0)
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
		FreeMarkedSamples(mixer);
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

 FreeMarkedSamples()
-----------------------------*/
void FreeMarkedSamples(struct Mixer* mixer)
{
	for (size_t i = 0; i < mixer->samples_no; i++)
	{
		if (((struct jaDictionaryItem**)mixer->marked_samples.data)[i] != NULL)
			jaDictionaryRemove(((struct jaDictionaryItem**)mixer->marked_samples.data)[i]);
	}
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
