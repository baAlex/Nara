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
static int sCallback(const void* raw_input, void* raw_output, unsigned long frames_no, const PaStreamCallbackTimeInfo* time,
                     PaStreamCallbackFlags status, void* raw_mixer)
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

			mixer->playlist[pl].cursor += 1;

			// End of sample
			if (mixer->playlist[pl].cursor >= mixer->playlist[pl].sample->length)
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
	{
		const char* sampling = NULL;

		if ((mixer = calloc(1, sizeof(struct Mixer))) == NULL)
		{
			jaStatusSet(st, "MixerCreate", STATUS_MEMORY_ERROR, NULL);
			return NULL;
		}

		if ((mixer->samples = jaDictionaryCreate(NULL)) == NULL)
			goto return_failure;

		if (jaCvarRetrieve(config, "mixer.volume", &mixer->cfg.volume, st) != 0 ||
		    jaCvarRetrieve(config, "mixer.frequency", &mixer->cfg.frequency, st) != 0 ||
		    jaCvarRetrieve(config, "mixer.channels", &mixer->cfg.channels, st) != 0 ||
		    jaCvarRetrieve(config, "mixer.max_sounds", &mixer->cfg.max_sounds, st) != 0 ||
		    jaCvarRetrieve(config, "mixer.sampling", &sampling, st) != 0)
			goto return_failure;

		if (strcmp(sampling, "linear") == 0)
			mixer->cfg.sampling = SRC_LINEAR;
		else if (strcmp(sampling, "zero_order") == 0)
			mixer->cfg.sampling = SRC_ZERO_ORDER_HOLD;
		else if (strcmp(sampling, "sinc_low") == 0)
			mixer->cfg.sampling = SRC_SINC_FASTEST;
		else if (strcmp(sampling, "sinc_medium") == 0)
			mixer->cfg.sampling = SRC_SINC_MEDIUM_QUALITY;
		else if (strcmp(sampling, "sinc_high") == 0)
			mixer->cfg.sampling = SRC_SINC_BEST_QUALITY;

		if (mixer->cfg.volume > 0.0 && mixer->cfg.max_sounds > 0)
			mixer->valid = true;
	}

	// PortAudio
	if (mixer->valid == true)
	{
		if ((errcode = Pa_Initialize()) != paNoError)
		{
			jaStatusSet(st, "MixerCreate", STATUS_ERROR, "Initialiting Portaudio: \"%s\"", Pa_GetErrorText(errcode));
			goto return_failure;
		}

		if ((errcode = Pa_OpenDefaultStream(&mixer->stream, 0, mixer->cfg.channels, paFloat32, (double)mixer->cfg.frequency,
		                                    paFramesPerBufferUnspecified, sCallback, mixer)) != paNoError)
		{
			jaStatusSet(st, "MixerCreate", STATUS_ERROR, "Opening stream: \"%s\"", Pa_GetErrorText(errcode));
			goto return_failure;
		}
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
	if (mixer->valid == true)
	{
		if (mixer->stream != NULL)
			Pa_StopStream(mixer->stream);

		Pa_Terminate();

		jaBufferClean(&mixer->marked_samples);
		jaBufferClean(&mixer->buffer);
	}

	jaDictionaryDelete(mixer->samples);
	free(mixer);
}


/*-----------------------------

 FreeMarkedSamples()
-----------------------------*/
inline void FreeMarkedSamples(struct Mixer* mixer)
{
	struct jaDictionaryItem** marked = mixer->marked_samples.data;

	for (size_t i = 0; i < mixer->samples_no; i++)
	{
		if (marked[i] != NULL)
		{
			jaDictionaryRemove(marked[i]);
			marked[i] = NULL;
		}
	}
}


/*-----------------------------

 Play2d
-----------------------------*/
static void sMixerStart(struct Mixer* mixer, struct jaStatus* st)
{
	PaError errcode = paNoError;

	if ((errcode = Pa_StartStream(mixer->stream)) != paNoError)
		jaStatusSet(st, "MixerStart", STATUS_ERROR, "Starting stream: \"%s\"", Pa_GetErrorText(errcode));
}

void Play2dFile(struct Mixer* mixer, float volume, enum PlayOptions options, const char* filename)
{
	struct jaDictionaryItem* item = NULL;
	struct Sample* sample = NULL;

	if (mixer->valid == false)
		return;

	if (mixer->started == false)
	{
		sMixerStart(mixer, NULL); // TODO, handle error
		mixer->started = true;
	}

	if ((item = jaDictionaryGet(mixer->samples, filename)) != NULL)
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

	if (mixer->valid == false)
		return;

	if (mixer->started == false)
	{
		sMixerStart(mixer, NULL); // TODO, handle error
		mixer->started = true;
	}

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

		// Bad luck, lets recycle
		if (ref_i == PLAY_LEN)
		{
			i = (mixer->last_index + 1) % PLAY_LEN;
			space = &mixer->playlist[i];
		}

		// Save index for the next play
		mixer->last_index = (i + 1);
	}

	// Yay
	sample->references += 1;

	space->active = false; // FIXME, if is an recycled space is necessary to set "references" appropriately

	space->cursor = 0;
	space->options = options;
	space->sample = sample;
	space->volume = volume;

	space->active = true;
}
