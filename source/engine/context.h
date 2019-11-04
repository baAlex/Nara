/*-----------------------------

 [context.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef CONTEXT_H
#define CONTEXT_H

	#include "image.h"
	#include "matrix.h"
	#include "status.h"
	#include "vector.h"

	#ifndef TEST
	#include "glad.h" // Before GLFW

	#define GLFW_INCLUDE_ES2
	#include <GLFW/glfw3.h>
	#else
	typedef unsigned GLuint;
	#endif

	#define ATTRIBUTE_POSITION 10
	#define ATTRIBUTE_UV 11

	enum Filter
	{
		FILTER_BILINEAR,
		FILTER_TRILINEAR,
		FILTER_PIXEL_BILINEAR,
		FILTER_PIXEL_TRILINEAR,
		FILTER_NONE
	};

	struct Program
	{
		GLuint glptr;
	};

	struct Vertex
	{
		struct Vector3 pos;
		struct Vector2 uv;
	};

	struct Vertices
	{
		GLuint glptr;
		size_t length; // In elements
	};

	struct Index
	{
		GLuint glptr;
		size_t length; // In elements
	};

	struct Texture
	{
		GLuint glptr;
	};

	struct ContextEvents
	{
		// Input
		bool a, b, x, y;
		bool lb, rb;
		bool view, menu, guide;
		bool ls, rs;

		struct { float h, v; } pad;
		struct { float h, v, t; } left_analog;
		struct { float h, v, t; } right_analog;

		struct
		{
			float x, y;
			bool a, b, c;
		} mouse;

		// Window
		bool resized;
		bool close;
		struct Vector2i window_size;
	};

	struct ContextOptions
	{
		char* caption;

		struct Vector2i window_size;
		struct Vector2i window_min_size;
		struct Vector3 clean_color;

		bool fullscreen;
	};

	struct Context* ContextCreate(struct ContextOptions options, struct Status* st);
	void ContextDelete(struct Context* context);
	void ContextUpdate(struct Context* context, struct ContextEvents* out_events);

	void SetTitle(struct Context* context, const char* title);
	void SetProgram(struct Context* context, const struct Program* program);
	void SetProjection(struct Context* context, struct Matrix4 matrix);
	void SetDiffuse(struct Context* context, const struct Texture* diffuse);
	void SetCameraLookAt(struct Context* context, struct Vector3 target, struct Vector3 origin);
	void SetCameraMatrix(struct Context* context, struct Matrix4 matrix, struct Vector3 origin);
	void SetHighlight(struct Context* context, struct Vector3 value);

	void Draw(struct Context* context, const struct Vertices* vertices, const struct Index* index);

	#define SetCamera(context, val, origin) _Generic((val), \
		struct Vector3: SetCameraLookAt, \
		struct Matrix4: SetCameraMatrix, \
		default: SetCameraLookAt \
	)(context, val, origin)

	int ProgramInit(struct Program* out, const char* vertex_code, const char* fragment_code, struct Status* st);
	void ProgramFree(struct Program*);

	int VerticesInit(struct Vertices* out, const struct Vertex* data, size_t length, struct Status* st);
	void VerticesFree(struct Vertices*);

	int IndexInit(struct Index* out, const uint16_t* data, size_t length, struct Status* st);
	void IndexFree(struct Index* index);

	int TextureInitImage(struct Texture* out, const struct Image* image, enum Filter, struct Status* st);
	int TextureInitFilename(struct Texture* out, const char* image_filename, enum Filter, struct Status* st);
	void TextureFree(struct Texture* texture);

	#define TextureInit(out, image, filter, st) _Generic((image), \
		const char*: TextureInitFilename, \
		char*: TextureInitFilename, \
		const struct Image*: TextureInitImage, \
		struct Image*: TextureInitImage, \
		default: TextureInitImage \
	)(out, image, filter, st)

#endif
