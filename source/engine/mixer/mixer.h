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
		PLAY_LOOP = 2
	};

	struct PlayRange
	{
		float min;
		float max;
	};

	struct Mixer* MixerCreate(const struct jaConfiguration*, struct jaStatus* st);
	void MixerDelete(struct Mixer* mixer);

	struct Sample* SampleCreate(struct Mixer* mixer, const char* filename, struct jaStatus* st);
	void SampleDelete(struct Sample*);

	void SetListener(struct Mixer* mixer, struct jaVector3 position);

	void PlayFile(struct Mixer*, enum PlayOptions, float volume, const char* filename);
	void PlaySample(struct Mixer*, enum PlayOptions, float volume, struct Sample*);

	void Play3dFile(struct Mixer*, enum PlayOptions, float volume, struct PlayRange, struct jaVector3 position, const char* filename);
	void Play3dSample(struct Mixer*, enum PlayOptions, float volume, struct PlayRange, struct jaVector3 position, struct Sample*);

#endif
