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
		struct
		{
			bool a, b, x, y;
			bool lb, rb;
			bool view, menu, guide;
			bool ls, rs;

		} button;

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
			float h, v;

		} pad;

		struct
		{
			bool exit;

		} system;

		struct
		{
			float betwen_frames; // In miliseconds
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
	void ContextSetCamera(struct Context* context, struct Matrix4 matrix);

	void ContextDraw(struct Context* context, const struct Vertices* vertices, const struct Index* index);

#endif
