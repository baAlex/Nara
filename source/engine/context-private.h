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
	#include "glad.h" // Before GLFW

	#define GLFW_INCLUDE_ES2
	#include <GLFW/glfw3.h>

	#define ATTRIBUTE_POSITION 10
	#define ATTRIBUTE_UV 11

	struct Context
	{
		GLFWwindow* window;
		struct ContextOptions options; // Set at initialization

		struct Vector2i window_size; // sResizeCallback()
		bool window_resized;         // sResizeCallback()

		// Draw routines
		struct Matrix4 projection;
		struct Matrix4 camera;
		struct Vector3 camera_origin;

		const struct Program* current_program;
		const struct Vertices* current_vertices;
		const struct Texture* current_texture;

		GLint u_projection;        // For current program
		GLint u_camera_projection; // "
		GLint u_camera_origin;     // "
		GLint u_texture;           // "
		GLint u_highlight;         // "

		// Input
		int active_gamepad;            // -1 if none
		struct ContextEvents keyboard; // KeyboardCallback()
		struct ContextEvents mouse;    // MousePosCallback(), MouseClickCallback()
		struct ContextEvents gamepad;  // InputStep()
		struct ContextEvents combined; // InputStep()
	};

	void InputStep(struct Context* context);

	void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	void MousePositionCallback(GLFWwindow* window, double x, double y);
	void MouseClickCallback(GLFWwindow* window, int button, int action, int mods);

	int FindGamedpad();

#endif
