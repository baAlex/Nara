/*-----------------------------

 [mixer.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef MIXER_H
#define MIXER_H

	#include "vector.h"
	#include "status.h"
	#include "sound.h"

	struct MixerOptions
	{
		int frequency; // 6000-48000 Hz
		int channels;  // 1-2
		float volume;  // 0.0-1.0
	};

	struct Mixer* MixerCreate(struct MixerOptions options, struct Status* st);
	void MixerDelete(struct Mixer* mixer);

	struct Sample* SampleCreate(struct Mixer* mixer, const char* filename, struct Status* st);
	void SampleDelete(struct Sample*);

	void PlaySample(struct Mixer*, float volume, struct Sample*);
	void PlayFile(struct Mixer*, float volume, const char* filename);

	#define Play(mixer, volume, obj) _Generic((obj), \
		const char*: PlayFile, \
		char*: PlayFile, \
		const struct Sample*: PlaySample, \
		default: PlaySample \
	)(mixer, volume, obj)

#endif
