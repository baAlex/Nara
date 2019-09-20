/*-----------------------------

 [mixer.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef MIXER_H
#define MIXER_H

	#include "vector.h"
	#include "status.h"

	struct MixerOptions
	{
		int frequency; // 6000-48000 Hz
		int channels;  // 1-2
		float volume;  // 0.0-1.0
	};

	struct Mixer* MixerCreate(struct MixerOptions options, struct Status* st);
	void MixerDelete(struct Mixer* mixer);

	void MixerPanic(struct Mixer* mixer);
	void MixerSetCamera(struct Mixer* mixer, struct Vector3 origin);

#endif
