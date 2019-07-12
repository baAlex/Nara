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
		struct Events events; // Callbacks write here

		int joystick_id;

		// Time calculation
		struct timespec one_second_counter;
		struct timespec last_update;
		long frame_no;
		long fps_counter;

		// Gl state
		const struct Program* current_program;

		GLint u_projection;         // For current program
		GLint u_camera_projection;  // "
		GLint u_camera_components;  // "
		GLint u_color_texture;      // "

		struct Matrix4 projection;
		struct Vector3 camera_components[2]; // 0 = Target, 1 = Origin
	};

#endif
