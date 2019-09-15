/*-----------------------------

 [context-private.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef CONTEXT_PRIVATE_H
#define CONTEXT_PRIVATE_H

	#include <math.h>
	#include <stdlib.h>
	#include <string.h>

	#include "context.h"

	#include <portaudio.h>

	#ifndef TINY_GL_H
	#define GLFW_INCLUDE_ES2
	#include <GLFW/glfw3.h>
	#endif

	struct Context
	{
		GLFWwindow* window;
		PaStream* stream;

		struct ContextOptions options; // Set at initialization
		bool audio_avaible;            // Set at initialization

		struct Vector2i window_size; // sResizeCallback()
		bool window_resized;         // sResizeCallback()

		// Draw routines
		struct Matrix4 projection;
		struct Matrix4 camera;
		struct Vector3 camera_origin;

		const struct Program* current_program;
		const struct Texture* current_diffuse;

		GLint u_projection;        // For current program
		GLint u_camera_projection; // "
		GLint u_camera_origin;     // "
		GLint u_color_texture;     // "

		// Input
		int active_gamepad;            // -1 if none
		struct ContextEvents keyboard; // KeyboardCallback()
		struct ContextEvents gamepad;  // InputStep()
		struct ContextEvents combined; // InputStep()
	};

	void InputStep(struct Context* context);
	void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	int FindGamedpad();

#endif
