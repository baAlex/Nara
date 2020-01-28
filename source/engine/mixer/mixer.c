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


static inline float sCalculateSpatialization(struct jaVector3 sound_pos, struct jaVector3 listener_pos)
{
	// TODO, hardcoded values
	const float max_distance = 700.0f;
	const float min_distance = 50.0f;

	const float d = jaVector3Distance(sound_pos, listener_pos);

	if (d < max_distance)
	{
		if (d < min_distance)
			return 1.0f;
		else
		{
			// The '15.0f + 1.0f' is an arbitrary number acting as an epsilon value
			// value. Check the file './resources/inverse-square-law.py'

			//printf("%f\n", 1.0f / powf(isql_feed, 2.0f));

			float isql_feed = ((d - min_distance) / (max_distance - min_distance)) * 15.0f + 1.0f;
			return 1.0f / powf(isql_feed, 2.0f);
		}
	}

	return 0.0f;
}


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
	const size_t channels_no = (size_t)mixer->cfg.channels;

	float* temp_data = NULL;
	float spatialization = 0.0f;

	for (float* output = raw_output; output < (float*)raw_output + (frames_no * channels_no); output += channels_no)
	{
		for (size_t ch = 0; ch < channels_no; ch++)
			output[ch] = 0.0f;

		// Sum all playlist
		for (struct PlayItem* playitem = mixer->playlist; playitem < (mixer->playlist + mixer->cfg.max_sounds); playitem++)
		{
			if (playitem->active == false)
				continue;

			// 3d Spatialization
			if ((playitem->options & PLAY_NO_3D) != PLAY_NO_3D)
				spatialization = sCalculateSpatialization(playitem->position, mixer->listener_pos);
			else
				spatialization = 1.0f;

			// Copy samples
			if (spatialization > 0.0f) // TODO, use epsilon?
			{
				temp_data = playitem->sample->data + (playitem->sample->channels * playitem->cursor);

				for (size_t ch = 0; ch < channels_no; ch++)
					output[ch] += (temp_data[ch % playitem->sample->channels] * playitem->volume * spatialization);
			}

			playitem->cursor += 1;

			// End of sample?
			if (playitem->cursor >= playitem->sample->length)
			{
				if ((playitem->options & PLAY_LOOP) == PLAY_LOOP)
				{
					playitem->cursor = 0;
					continue;
				}

				playitem->active = false;
				playitem->sample->references -= 1;
			}
		}

		for (size_t ch = 0; ch < channels_no; ch++)
		{
			output[ch] /= DspLimiterGain(&mixer->limiter, ch, output[ch]);

			output[ch] = jaClamp(output[ch], -1.0f, +1.0f);
			output[ch] *= mixer->cfg.volume;
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

	// Mixer and sampling module initialization (both are tied)
	{
		struct Cfg temp_cfg = {0};
		const char* sampling = NULL;

		if (jaCvarRetrieve(config, "mixer.volume", &temp_cfg.volume, st) != 0 ||
		    jaCvarRetrieve(config, "mixer.frequency", &temp_cfg.frequency, st) != 0 ||
		    jaCvarRetrieve(config, "mixer.channels", &temp_cfg.channels, st) != 0 ||
		    jaCvarRetrieve(config, "mixer.max_sounds", &temp_cfg.max_sounds, st) != 0 ||
		    jaCvarRetrieve(config, "mixer.sampling", &sampling, st) != 0)
			goto return_failure;

		if (strcmp(sampling, "linear") == 0)
			temp_cfg.sampling = SRC_LINEAR;
		else if (strcmp(sampling, "zero_order") == 0)
			temp_cfg.sampling = SRC_ZERO_ORDER_HOLD;
		else if (strcmp(sampling, "sinc_low") == 0)
			temp_cfg.sampling = SRC_SINC_FASTEST;
		else if (strcmp(sampling, "sinc_medium") == 0)
			temp_cfg.sampling = SRC_SINC_MEDIUM_QUALITY;
		else if (strcmp(sampling, "sinc_high") == 0)
			temp_cfg.sampling = SRC_SINC_BEST_QUALITY;
		else
			temp_cfg.sampling = SRC_SINC_FASTEST;

		if ((mixer = calloc(1, sizeof(struct Mixer) + sizeof(struct PlayItem) * (size_t)temp_cfg.max_sounds)) == NULL)
		{
			jaStatusSet(st, "MixerCreate", STATUS_MEMORY_ERROR, NULL);
			return NULL;
		}

		if ((mixer->samples = jaDictionaryCreate(NULL)) == NULL)
			goto return_failure;

		mixer->cfg = temp_cfg;

		if (mixer->cfg.volume > 0.0 && mixer->cfg.max_sounds > 0)
			mixer->valid = true;
	}

	// Dsp modules
	if (DspLimiterInit(config, &mixer->limiter, st) != 0)
		goto return_failure;

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
		jaBufferClean(&mixer->buffer);
	}

	jaDictionaryDelete(mixer->samples);
	free(mixer);
}


/*-----------------------------

 SetListener()
-----------------------------*/
inline void SetListener(struct Mixer* mixer, struct jaVector3 position)
{
	mixer->listener_pos = position;
}


/*-----------------------------

 Play
-----------------------------*/
static void sMixerStart(struct Mixer* mixer, struct jaStatus* st)
{
	PaError errcode = paNoError;

	if ((errcode = Pa_StartStream(mixer->stream)) != paNoError)
		jaStatusSet(st, "MixerStart", STATUS_ERROR, "Starting stream: \"%s\"", Pa_GetErrorText(errcode));
}

void PlayFile(struct Mixer* mixer, float volume, enum PlayOptions options, struct jaVector3 position, const char* filename)
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
		PlaySample(mixer, volume, options, position, (struct Sample*)item->data);
	else
	{
		printf("Creating sample for '%s'...\n", filename);

		if ((sample = SampleCreate(mixer, filename, NULL)) != NULL)
			PlaySample(mixer, volume, options, position, sample);
	}
}

void PlaySample(struct Mixer* mixer, float volume, enum PlayOptions options, struct jaVector3 position, struct Sample* sample)
{
	struct PlayItem* playitem = NULL;

	if (mixer->valid == false)
		return;

	if (mixer->started == false)
	{
		sMixerStart(mixer, NULL); // TODO, handle error
		mixer->started = true;
	}

	// Find an item in the playlist
	{
		int base_i = 0;
		int i = 0;

		for (base_i = 0; base_i < mixer->cfg.max_sounds; base_i++)
		{
			i = (mixer->last_index + base_i) % mixer->cfg.max_sounds;

			if (mixer->playlist[i].active == false)
			{
				playitem = &mixer->playlist[i];
				break;
			}
		}

		// Bad luck, lets recycle
		if (base_i == mixer->cfg.max_sounds)
		{
			i = (mixer->last_index + 1) % mixer->cfg.max_sounds;
			playitem = &mixer->playlist[i];
		}

		// Save index for the next play
		mixer->last_index = (i + 1);
	}

	// Yay
	if (playitem->active == true)
	{
		playitem->active = false;
		playitem->sample->references -= 1;
	}

	sample->references += 1;

	playitem->cursor = 0;
	playitem->options = options;
	playitem->sample = sample;
	playitem->volume = volume;
	playitem->position = position;

	playitem->active = true;
}
