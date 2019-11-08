/*-----------------------------

 [timer.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef TIMER_H
#define TIMER_H

	#include <stdbool.h>
	#include <time.h>

	struct Timer
	{
		bool one_second;
		long frame_number;
		int frames_per_second;
		double miliseconds_betwen;

		struct timespec last_update;

		// Private
		struct timespec second_counter;
		long fps_counter;
	};

	void TimerInit(struct Timer* timer);
	void TimerUpdate(struct Timer* timer);

#endif
