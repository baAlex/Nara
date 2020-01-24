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


References:

Dimitrios Giannolis, et al. (2012), «Digital Dynamic Range Compressor Design—
A Tutorial and Analysis», J. Audio Eng. Soc., Vol. 60, No. 6, Queen Mary
University of London, London, UK.

Hämäläinen Perttu (2002), «Smoothing Of The Control Signal Without Clipped
Output In Digital Peak Limiters», Proc. of the 5th Int. Conference on
Digital Audio Effects (DAFx-02), Hamburg, Germany.

Roelandts (2016), «Low-Pass Single-Pole IIR Filter»:
https://tomroelandts.com/articles/low-pass-single-pole-iir-filter

Cavaliere (2017), «How to calculate a first order IIR filter in 5 minutes»:
https://www.monocilindro.com/2017/04/08/how-to-implement-a-1st-order-iir-filter-in-5-minutes/

-----------------------------*/

#include "private.h"


static inline float sFirstOrderIIRLowPass(float k, float input, float previous_sample)
{
	return (k * input + (1.0f - k) * previous_sample);
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

	for (float* output = raw_output; output < (float*)raw_output + (frames_no * channels_no); output += channels_no)
	{
		for (size_t ch = 0; ch < channels_no; ch++)
			output[ch] = 0.0f;

		// Sum all playlist
		for (struct PlayItem* playitem = mixer->playlist; playitem < (mixer->playlist + mixer->cfg.max_sounds); playitem++)
		{
			if (playitem->active == false)
				continue;

			temp_data = playitem->sample->data + (playitem->sample->channels * playitem->cursor);

			for (size_t ch = 0; ch < channels_no; ch++)
				output[ch] += (temp_data[ch % playitem->sample->channels] * playitem->volume);

			playitem->cursor += 1;

			// End of sample
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
#if 1
			// TODO, hardcoded K values
			float env = 0;

			// Limiter attack/release
			if (fabsf(output[ch]) > mixer->last_limiter_env[ch])
				env = sFirstOrderIIRLowPass(0.9f, fabsf(output[ch]), fabsf(mixer->last_limiter_env[ch]));
			else
				env = sFirstOrderIIRLowPass(0.001f, fabsf(output[ch]), fabsf(mixer->last_limiter_env[ch]));

			mixer->last_limiter_env[ch] = env;

			// Limiter gain
			output[ch] /= (env > mixer->cfg.limiter_threshold) ? env / mixer->cfg.limiter_threshold : 1.0f;
#endif

			// Clipping and finally, volume
			// if (output[ch] < -1.0f || output[ch] > +1.0f)
			// 	printf("Clipping! %i\n", rand());

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

	// Initialization
	{
		struct Cfg temp_cfg = {0};
		const char* sampling = NULL;

		if (jaCvarRetrieve(config, "mixer.volume", &temp_cfg.volume, st) != 0 ||
		    jaCvarRetrieve(config, "mixer.frequency", &temp_cfg.frequency, st) != 0 ||
		    jaCvarRetrieve(config, "mixer.channels", &temp_cfg.channels, st) != 0 ||
		    jaCvarRetrieve(config, "mixer.max_sounds", &temp_cfg.max_sounds, st) != 0 ||
		    jaCvarRetrieve(config, "mixer.sampling", &sampling, st) != 0 ||
		    jaCvarRetrieve(config, "mixer.limiter.threshold", &temp_cfg.limiter_threshold, st) != 0 ||
		    jaCvarRetrieve(config, "mixer.limiter.attack", &temp_cfg.limiter_attack, st) != 0 ||
		    jaCvarRetrieve(config, "mixer.limiter.release", &temp_cfg.limiter_release, st) != 0)
			goto return_failure;

		// temp_cfg.limiter_attack = ???;
		// temp_cfg.limiter_release = ???;

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

	playitem->active = true;
}
