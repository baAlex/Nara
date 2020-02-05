/*-----------------------------

 [context.h]
 - Alexander Brandt 2019-2020
-----------------------------*/

#ifndef CONTEXT_H
#define CONTEXT_H

	#include "japan-aabounding.h"
	#include "japan-configuration.h"
	#include "japan-image.h"
	#include "japan-matrix.h"
	#include "japan-status.h"
	#include "japan-vector.h"

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
		struct jaVector3 pos;
		struct jaVector2 uv;
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
		struct jaVector2i window_size;
	};

	struct Context* ContextCreate(const struct jaConfiguration*, const char* caption, struct jaStatus* st);
	void ContextDelete(struct Context* context);
	void ContextUpdate(struct Context* context, struct ContextEvents* out_events);

	void SetFunctionKeyCallback(struct Context* context, int key, void* user_data,
								void (*)(const struct Context*, const struct ContextEvents*, bool, void*));

	void SetTitle(struct Context* context, const char* title);
	void SetProgram(struct Context* context, const struct Program* program);
	void SetVertices(struct Context* context, const struct Vertices* vertices);
	void SetTexture(struct Context* context, int unit, const struct Texture* texture);
	void SetProjection(struct Context* context, struct jaMatrix4 matrix);
	void SetHighlight(struct Context* context, struct jaVector3 value);
	void SetCameraLookAt(struct Context* context, struct jaVector3 target, struct jaVector3 origin);
	void SetCameraMatrix(struct Context* context, struct jaMatrix4 matrix, struct jaVector3 origin);

	struct jaVector2i GetWindowSize(const struct Context* context);
	int GetDrawCalls(const struct Context*);
	struct jaVector3 GetCameraOrigin(const struct Context*);

	int TakeScreenshot(const struct Context* context, const char* filename, struct jaStatus* st);
	void Draw(struct Context* context, const struct Index* index);
	void DrawAABB(struct Context* context, struct jaAABBox box, struct jaVector3 pos);

	#define SetCamera(context, val, origin) \
		_Generic((val), \
			struct jaVector3: SetCameraLookAt, \
			struct jaMatrix4: SetCameraMatrix, \
			default: SetCameraLookAt \
		)(context, val, origin)

	int ProgramInit(const char* vertex_code, const char* fragment_code, struct Program* out, struct jaStatus* st);
	void ProgramFree(struct Program* program);

	int VerticesInit(const struct Vertex* data, uint16_t length, struct Vertices* out, struct jaStatus* st);
	void VerticesFree(struct Vertices* vertices);

	int IndexInit(const uint16_t* data, size_t length, struct Index* out, struct jaStatus* st);
	void IndexFree(struct Index* index);

	int TextureInitImage(const struct Context*, const struct jaImage* image, struct Texture* out, struct jaStatus* st);
	int TextureInitFilename(const struct Context*, const char* image_filename, struct Texture* out, struct jaStatus* st);
	void TextureFree(struct Texture* texture);

	#define TextureInit(context, src, out, st) \
		_Generic((src), \
			const char*: TextureInitFilename, \
			char*: TextureInitFilename, \
			const struct jaImage*: TextureInitImage, \
			struct jaImage*: TextureInitImage, \
			default: TextureInitImage \
		)(context, src, out, st)

#endif
