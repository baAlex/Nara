/*-----------------------------

 [context-gl.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef CONTEXT_GL
#define CONTEXT_GL

	#include <stddef.h>
	#include <stdbool.h>

	#include "vector.h"
	#include "status.h"

	#define GLFW_INCLUDE_ES2 // TODO: Hardcoded
	#include <GLFW/glfw3.h>

	#define ATTRIBUTE_POSITION 10
	#define ATTRIBUTE_COLOR 11
	//#define ATTRIBUTE_UV 12

	struct Vertex
	{
		struct Vector3 pos;
		struct Vector4 col;
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

	enum ScaleMode
	{
		SCALE_MODE_ASPECT = 0,
		SCALE_MODE_STEPS,
		SCALE_MODE_FIXED,
		SCALE_MODE_STRETCH
	};

	struct Program* ProgramCreate(const char* vertex_code, const char* fragment_code, struct Status* st);
	void ProgramDelete(struct Program*);

	struct Vertices* VerticesCreate(const struct Vertex* data, size_t length, struct Status* st);
	void VerticesDelete(struct Vertices*);

	struct Index* IndexCreate(const uint16_t* data, size_t length, struct Status* st);
	void IndexDelete(struct Index*);

	void ViewportResize(enum ScaleMode mode, int new_w, int new_h, int aspect_w, int aspect_h);

#endif
