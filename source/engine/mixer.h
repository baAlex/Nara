/*-----------------------------

 [mixer.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef MIXER_H
#define MIXER_H

	#include "vector.h"
	#include "status.h"
	#include "sound.h"

	enum PlayOptions
	{
		PLAY_NORMAL = 0,
		PLAY_LOOP = 2
	};

	struct MixerOptions
	{
		int frequency; // 6000-48000 Hz
		int channels;  // 1-2
		float volume;  // 0.0-1.0
	};

	struct Mixer* MixerCreate(struct MixerOptions options, struct Status* st);
	void MixerDelete(struct Mixer* mixer);

	int MixerStart(struct Mixer* mixer, struct Status* st);
	void MixerStop(struct Mixer* mixer); // TODO

	struct Sample* SampleCreate(struct Mixer* mixer, const char* filename, struct Status* st);
	void SampleDelete(struct Sample*);

	void Play2dSample(struct Mixer*, float volume, enum PlayOptions, struct Sample*);
	void Play2dFile(struct Mixer*, float volume, enum PlayOptions, const char* filename);

	#define Play2d(mixer, volume, options, obj) _Generic((obj), \
		const char*: Play2dFile, \
		char*: Play2dFile, \
		const struct Sample*: Play2dSample, \
		default: Play2dSample \
	)(mixer, volume, options, obj)

#endif
