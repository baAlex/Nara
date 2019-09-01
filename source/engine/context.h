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
	#include "tiny-gl.h"

	struct WindowSpecifications
	{
		struct Vector2i size;
		bool resized;
		bool close;
	};

	struct TimeSpecifications
	{
		bool one_second;
		long frame_number;
		int frames_per_second;
		double miliseconds_betwen;
	};

	struct InputSpecifications
	{
		bool a, b, x, y;
		bool lb, rb;
		bool view, menu, guide;
		bool ls, rs;

		struct { float h, v; } pad;
		struct { float h, v, t; } left_analog;
		struct { float h, v, t; } right_analog;
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
	void ContextUpdate(struct Context* context, struct WindowSpecifications* out_window,
	                   struct TimeSpecifications* out_time, struct InputSpecifications* out_input);

	void ContextSetProgram(struct Context* context, const struct Program* program);
	void ContextSetProjection(struct Context* context, struct Matrix4 matrix);
	void ContextSetDiffuse(struct Context* context, const struct Texture* diffuse);
	void ContextSetCameraLookAt(struct Context* context, struct Vector3 target, struct Vector3 origin);
	void ContextSetCameraMatrix(struct Context* context, struct Matrix4 matrix, struct Vector3 origin);

	void ContextDraw(struct Context* context, const struct Vertices* vertices, const struct Index* index);

	#define ContextSetCamera(context, val, origin) _Generic((val), \
		struct Vector3: ContextSetCameraLookAt, \
		struct Matrix4: ContextSetCameraMatrix, \
		default: ContextSetCameraLookAt \
	)(context, val, origin)

#endif
