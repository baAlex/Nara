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

	#define GLFW_INCLUDE_ES2
	#include <GLFW/glfw3.h>

	#define ATTRIBUTE_POSITION 10
	#define ATTRIBUTE_UV 11


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
		struct InputSpecifications specs;
		int active_gamepad; // -1 if none
	};

	void ContextInputInitialization(struct ContextInput* state);
	void ContextInputStep(struct ContextInput* state);


	struct ContextOpenGl
	{
		struct Matrix4 projection;
		struct Vector3 camera_components[2]; // 0 = Target, 1 = Origin

		const struct Program* current_program;

		GLint u_projection;        // For current program
		GLint u_camera_projection; // "
		GLint u_camera_components; // "
		GLint u_color_texture;     // "
	};

	void ContextOpenGlInitialization(struct ContextOpenGl* state, struct Vector3 clean_color);
	void ContextOpenGlStep(struct ContextOpenGl* state);

	void ViewportResize(enum ScaleMode mode, int new_w, int new_h, int aspect_w, int aspect_h);
	void SetProgram(struct ContextOpenGl*, const struct Program* program);
	void SetProjection(struct ContextOpenGl*, struct Matrix4 matrix);
	void SetCamera(struct ContextOpenGl*, struct Vector3 target, struct Vector3 origin);
	void Draw(const struct Vertices* vertices, const struct Index* index, const struct Texture* color);


	struct Context
	{
		GLFWwindow* window;
		struct Vector2i window_size;
		bool window_resized;

		struct ContextOptions options;

		// Modules:
		struct ContextOpenGl opengl;
		struct ContextInput input;
		struct ContextTime time;
	};

	struct Program
	{
		GLuint ptr;
	};

	struct Vertices
	{
		GLuint ptr;
		size_t length; // In elements
	};

	struct Index
	{
		GLuint ptr;
		size_t length; // In elements
	};

	struct Texture
	{
		GLuint ptr;
	};

#endif
