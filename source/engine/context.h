/*-----------------------------

 [context.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef CONTEXT_H
#define CONTEXT_H

	#include "image.h"
	#include "status.h"
	#include "vector.h"
	#include "matrix.h"
	#include "options.h"

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
		unsigned int glptr;
	};

	struct Vertex
	{
		struct Vector3 pos;
		struct Vector2 uv;
	};

	struct Vertices
	{
		unsigned int glptr;
		uint16_t length; // In elements
	};

	struct Index
	{
		unsigned int glptr;
		size_t length; // In elements
	};

	struct Texture
	{
		unsigned int glptr;
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

		// Window
		bool resized;
		bool close;
		struct Vector2i window_size;
	};

	struct Context* ContextCreate(const struct Options* options, const char* caption, struct Status* st);
	void ContextDelete(struct Context* context);
	void ContextUpdate(struct Context* context, struct ContextEvents* out_events);

	void SetFunctionKeyCallback(struct Context* context, int key, void (*)(const struct Context*, const struct ContextEvents*, bool press));

	void SetTitle(struct Context* context, const char* title);
	void SetProgram(struct Context* context, const struct Program* program);
	void SetVertices(struct Context* context, const struct Vertices* vertices);
	void SetTexture(struct Context* context, int unit, const struct Texture* texture);
	void SetProjection(struct Context* context, struct Matrix4 matrix);
	void SetHighlight(struct Context* context, struct Vector3 value);
	void SetCameraLookAt(struct Context* context, struct Vector3 target, struct Vector3 origin);
	void SetCameraMatrix(struct Context* context, struct Matrix4 matrix, struct Vector3 origin);

	struct Vector2i GetWindowSize(const struct Context* context);

	void Draw(struct Context* context, const struct Index* index);

	#define SetCamera(context, val, origin) _Generic((val), \
		struct Vector3: SetCameraLookAt, \
		struct Matrix4: SetCameraMatrix, \
		default: SetCameraLookAt \
	)(context, val, origin)

	int ProgramInit(struct Program* out, const char* vertex_code, const char* fragment_code, struct Status* st);
	void ProgramFree(struct Program* program);

	int VerticesInit(struct Vertices* out, const struct Vertex* data, uint16_t length, struct Status* st);
	void VerticesFree(struct Vertices* vertices);

	int IndexInit(struct Index* out, const uint16_t* data, size_t length, struct Status* st);
	void IndexFree(struct Index* index);

	int TextureInitImage(struct Texture* out, const struct Image* image, enum Filter filter, struct Status* st);
	int TextureInitFilename(struct Texture* out, const char* image_filename, enum Filter filter, struct Status* st);
	void TextureFree(struct Texture* texture);

	#define TextureInit(out, image, filter, st) _Generic((image), \
		const char*: TextureInitFilename, \
		char*: TextureInitFilename, \
		const struct Image*: TextureInitImage, \
		struct Image*: TextureInitImage, \
		default: TextureInitImage \
	)(out, image, filter, st)

#endif
