/*-----------------------------

 [mixer.h]
 - Alexander Brandt 2019-2020
-----------------------------*/

#ifndef MIXER_H
#define MIXER_H

	#include "japan-vector.h"
	#include "japan-status.h"
	#include "japan-sound.h"
	#include "japan-configuration.h"

	enum PlayOptions
	{
		PLAY_NORMAL = 0,
		PLAY_LOOP = 2,
		PLAY_NO_3D = 4
	};

	struct Mixer* MixerCreate(const struct jaConfiguration*, struct jaStatus* st);
	void MixerDelete(struct Mixer* mixer);

	struct Sample* SampleCreate(struct Mixer* mixer, const char* filename, struct jaStatus* st);
	void SampleDelete(struct Sample*);

	void SetListener(struct Mixer* mixer, struct jaVector3 position);

	void PlaySample(struct Mixer*, float volume, enum PlayOptions, struct jaVector3 position, struct Sample*);
	void PlayFile(struct Mixer*, float volume, enum PlayOptions, struct jaVector3 position, const char* filename);

	#define Play(mixer, volume, options, pos, obj) _Generic((obj), \
		const char*: PlayFile, \
		char*: PlayFile, \
		const struct Sample*: PlaySample, \
		default: PlaySample \
	)(mixer, volume, options, pos, obj)

#endif
