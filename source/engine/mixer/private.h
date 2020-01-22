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

	#define PLAY_LEN 8

	struct ToPlay
	{
		bool active;
		size_t cursor;

		struct Sample* sample;
		float volume;

		enum PlayOptions options;
	};

	struct Mixer
	{
		struct
		{
			float volume;
			int frequency;
			int channels;
			int max_sounds;
			int sampling;
		} cfg;

		bool valid;
		bool started;

		PaStream* stream;

		struct jaDictionary* samples;
		struct jaBuffer marked_samples; // To free them
		size_t samples_no;

		struct jaBuffer buffer; // SampleCreate()

		struct ToPlay playlist[PLAY_LEN];
		size_t last_index; // Play2dSample()
	};

	struct Sample
	{
		struct jaDictionaryItem* item;

		int references;
		bool to_delete;

		size_t length;
		size_t channels;
		float data[];
	};

	void FreeMarkedSamples(struct Mixer* mixer);

#endif
