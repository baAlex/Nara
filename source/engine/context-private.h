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
		struct InputSpecifications specs; // The following ones combined

		struct InputSpecifications key_specs;
		struct InputSpecifications mouse_specs;

		int active_gamepad; // -1 if none
	};

	void ContextInputInitialization(struct ContextInput* state);
	void ContextInputStep(struct ContextInput* state);

	void ReceiveKeyboardKey(struct ContextInput* state, int key, int action);

	struct ContextOpenGl
	{
		struct Matrix4 projection;
		struct Matrix4 camera;
		struct Vector3 camera_origin;

		const struct Program* current_program;

		GLint u_projection;        // For current program
		GLint u_camera_projection; // "
		GLint u_camera_origin;     // "
		GLint u_color_texture;     // "
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

	void ContextOpenGlInitialization(struct ContextOpenGl* state, struct Vector3 clean_color);
	void ContextOpenGlStep(struct ContextOpenGl* state);

	void ViewportResize(enum ScaleMode mode, int new_w, int new_h, int aspect_w, int aspect_h);
	void SetProgram(struct ContextOpenGl*, const struct Program* program);
	void SetProjection(struct ContextOpenGl*, struct Matrix4 matrix);
	void SetCamera(struct ContextOpenGl*, struct Vector3 target, struct Vector3 origin);
	void SetCameraAsMatrix(struct ContextOpenGl*, struct Matrix4 matrix, struct Vector3 origin);
	void Draw(const struct Vertices* vertices, const struct Index* index, const struct Texture* color);

#endif
