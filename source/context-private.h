/*-----------------------------

 [context-private.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef CONTEXT_PRIVATE_H
#define CONTEXT_PRIVATE_H

	#include <string.h>
	#include <stdlib.h>
	#include <time.h>

	#include "context.h"

	struct Context
	{
		GLFWwindow* window;

		struct ContextOptions options;
		struct Events events;

		// Time calculation
		struct timespec one_second_counter;
		struct timespec last_update;
		long frame_no;
		long fps_counter;

		// Gl state
		const struct Program* current_program;

		struct Matrix4 projection;
		struct Matrix4 camera;
	};

#endif
