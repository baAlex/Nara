/*-----------------------------

 [context-private.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef CONTEXT_PRIVATE_H
#define CONTEXT_PRIVATE_H

	#include <string.h>
	#include <stdlib.h>
	#include "context.h"

	struct Context
	{
		GLFWwindow* window;

		struct ContextOptions options;
		struct Events events;

		const struct Program* current_program;

		struct Matrix4 projection;
		struct Matrix4 camera;
	};

#endif
