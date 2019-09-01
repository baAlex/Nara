/*-----------------------------

 [context-private.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef CONTEXT_PRIVATE_H
#define CONTEXT_PRIVATE_H

	#include <string.h>
	#include <stdlib.h>
	#include <math.h>
	#include <time.h>

	#include "context.h"

	struct ContextTime
	{
		struct TimeSpecifications specs;

		struct timespec last_update;
		struct timespec second_counter;
		long fps_counter;
	};

	void ContextTimeInitialization(struct ContextTime* state);
	void ContextTimeStep(struct ContextTime* state);


	struct ContextInput
	{
		struct InputSpecifications specs; // The following ones combined

		struct InputSpecifications key_specs;
		struct InputSpecifications mouse_specs;

		int active_gamepad; // -1 if none
	};

	void ContextInputInitialization(struct ContextInput* state);
	void ContextInputStep(struct ContextInput* state);

	void ReceiveKeyboardKey(struct ContextInput* state, int key, int action);

#endif
