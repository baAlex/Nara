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

#include "mixer.h"
#include "misc.h"
#include <portaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Mixer
{
	PaStream* stream;
	struct MixerOptions options;

	struct Vector3 origin;
};

struct StereoFrame
{
	float left;
	float right;
};


/*-----------------------------

 sCallback
-----------------------------*/
static int sCallbackStereo(const void* input, void* output, unsigned long frames_no,
                           const PaStreamCallbackTimeInfo* time, PaStreamCallbackFlags status, void* data)
{
	(void)input;
	(void)time;
	(void)status;
	(void)data;

	struct StereoFrame* frame = output;

	for (unsigned long i = 0; i < frames_no; i++)
	{
		frame[i].left = 0.0f;
		frame[i].right = 0.0f;
	}

	return paContinue;
}

static int sCallbackMono(const void* input, void* output, unsigned long frames_no, const PaStreamCallbackTimeInfo* time,
                         PaStreamCallbackFlags status, void* data)
{
	(void)input;
	(void)time;
	(void)status;
	(void)data;

	float* frame = output;

	for (unsigned long i = 0; i < frames_no; i++)
	{
		frame[i] = 0.0f;
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
	PaStreamCallback* callback = (options.channels == 1) ? sCallbackMono : sCallbackStereo;

	if (Pa_OpenDefaultStream(&mixer->stream, 0, options.channels, paFloat32, (double)options.frequency,
	                         paFramesPerBufferUnspecified, callback, mixer) != paNoError)
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
	free(mixer);
}


/*-----------------------------

 MixerPanic()
-----------------------------*/
void MixerPanic(struct Mixer* mixer)
{
	(void)mixer;
}


/*-----------------------------

 MixerSetCamera()
-----------------------------*/
inline void MixerSetCamera(struct Mixer* mixer, struct Vector3 origin)
{
	mixer->origin = origin;
}
