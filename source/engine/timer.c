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

 [timer.c]
 - Alexander Brandt 2019
-----------------------------*/

#include "timer.h"
#include <string.h>


void TimerInit(struct Timer* timer)
{
	memset(timer, 0, sizeof(struct Timer));
	timespec_get(&timer->last_update, TIME_UTC);
	timespec_get(&timer->second_counter, TIME_UTC);
}


void TimerStep(struct Timer* timer)
{
	struct timespec current_time = {0};
	timespec_get(&current_time, TIME_UTC);

	timer->frame_number++;

	timer->miliseconds_betwen = current_time.tv_nsec / 1000000.0 + current_time.tv_sec * 1000.0;
	timer->miliseconds_betwen -= timer->last_update.tv_nsec / 1000000.0 + timer->last_update.tv_sec * 1000.0;
	timer->one_second = false;

	if (current_time.tv_sec > timer->second_counter.tv_sec)
	{
		timer->second_counter = current_time;
		timer->one_second = true;

		timer->frames_per_second = timer->frame_number - timer->fps_counter;
		timer->fps_counter = timer->frame_number;
	}

	timer->last_update = current_time;
}