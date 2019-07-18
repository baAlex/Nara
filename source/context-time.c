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

 [context-time.c]
 - Alexander Brandt 2019
-----------------------------*/

#include "context-private.h"


/*-----------------------------

 ContextTimeInitialization()
-----------------------------*/
void ContextTimeInitialization(struct ContextTime* state)
{
	memset(state, 0, sizeof(struct ContextTime));
	timespec_get(&state->last_update, TIME_UTC);
	timespec_get(&state->second_counter, TIME_UTC);
}


/*-----------------------------

 ContextTimeStep()
-----------------------------*/
void ContextTimeStep(struct ContextTime* state)
{
	struct timespec current_time = {0};
	timespec_get(&current_time, TIME_UTC);

	state->specs.frame_number++;

	state->specs.miliseconds_betwen = current_time.tv_nsec / 1000000.0 + current_time.tv_sec * 1000.0;
	state->specs.miliseconds_betwen -= state->last_update.tv_nsec / 1000000.0 + state->last_update.tv_sec * 1000.0;
	state->specs.one_second = false;

	if (current_time.tv_sec > state->second_counter.tv_sec)
	{
		state->second_counter = current_time;
		state->specs.one_second = true;

		state->specs.frames_per_second = state->specs.frame_number - state->fps_counter;
		state->fps_counter = state->specs.frame_number;
	}

	state->last_update = current_time;
}
