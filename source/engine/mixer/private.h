/*-----------------------------

 [mixer/private.h]
 - Alexander Brandt 2019-2020
-----------------------------*/

#ifndef MIXER_PRIVATE_H
#define MIXER_PRIVATE_H

	#include <math.h>
	#include <portaudio.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <time.h>
	#include <stdbool.h>

	#include "japan-buffer.h"
	#include "japan-dictionary.h"
	#include "japan-utilities.h"

	#include "mixer.h"
	#include "samplerate.h"
	#include "../misc.h"

	struct DspLimiter
	{
		float threshold;
		float attack;
		float release;

		float last_envelope_sample[2]; // TODO, hardcoded stereo
	};

	struct Sample
	{
		struct jaDictionaryItem* item;

		int references;

		size_t length;
		size_t channels;
		float data[];
	};

	struct PlayItem
	{
		bool active;
		size_t cursor;

		struct Sample* sample;
		float volume;

		enum PlayOptions options;

		// 3d sounds
		bool is_3d;
		struct PlayRange range;    // SetListener()
		struct jaVector3 position; // SetListener()
		float gain_3d[2];          // TODO, hardcoded stereo
	};

	struct Cfg
	{
		float volume;
		int frequency;
		int channels;
		int sampling;
		int max_sounds;
	};

	struct Mixer
	{
		// Dsp
		struct DspLimiter limiter;

		// Samples
		struct jaBuffer buffer;
		struct jaDictionary* samples;
		size_t samples_no;

		// Mixer
		struct Cfg cfg;
		bool valid;
		bool started;

		PaStream* stream;

		struct jaVector3 listener_pos;
		struct jaVector3 listener_left_axy;

		int last_index;
		struct PlayItem playlist[];
	};


	int DspLimiterInit(const struct jaConfiguration* config, struct DspLimiter* limiter, struct jaStatus* st);
	float DspLimiterGain(struct DspLimiter* limiter, size_t ch, float input);

#endif
