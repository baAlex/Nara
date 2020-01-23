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

	struct PlayItem
	{
		bool active;
		size_t cursor;

		struct Sample* sample;
		float volume;

		enum PlayOptions options;
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
		// Mixer
		struct Cfg cfg;
		bool valid;
		bool started;

		PaStream* stream;

		// Samples
		struct jaBuffer buffer;
		struct jaDictionary* samples;
		size_t samples_no;

		// Playlist
		int last_index;
		struct PlayItem playlist[];
	};

	struct Sample
	{
		struct jaDictionaryItem* item;

		int references;

		size_t length;
		size_t channels;
		float data[];
	};

#endif
