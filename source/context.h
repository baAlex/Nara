/*-----------------------------

 [context.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef CONTEXT_H
#define CONTEXT_H

	#include "context-gl.h"
	#include "matrix.h"

	struct Context;

	struct VectorInt
	{
		int x;
		int y;
		int z;
	};

	struct Events
	{
		bool a, b, x, y;
		bool lb, rb;
		bool view, menu, guide;
		bool ls, rs;

		struct
		{
			float h, v;
		} pad;

		struct
		{
			float h, v, t;
		} left_analog;

		struct
		{
			float h, v, t;
		} right_analog;

		struct
		{
			bool resize;
			int width;
			int height;

			bool close;

		} window;

		struct
		{
			double betwen_frames; // In miliseconds
			long frame_no;
			bool one_second;

		} time;
	};

	struct ContextOptions
	{
		char* caption;

		struct VectorInt window_size;
		struct VectorInt windows_min_size;
		struct Vector3 clean_color;

		enum ScaleMode scale_mode;
		union
		{
			struct VectorInt aspect;
			struct VectorInt steps_size;
		};

		bool fullscreen;
	};

	struct Context* ContextCreate(struct ContextOptions options, struct Status* st);
	void ContextUpdate(struct Context* context, struct Events* out_events);
	void ContextDelete(struct Context* context);

	void ContextSetProgram(struct Context* context, const struct Program* program);
	void ContextSetProjection(struct Context* context, struct Matrix4 matrix);
	void ContextSetCamera(struct Context* context, struct Vector3 target, struct Vector3 origin);

	void ContextDraw(struct Context* context, const struct Vertices* vertices, const struct Index* index, const struct Texture* color);

#endif
