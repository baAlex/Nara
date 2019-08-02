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

	struct Context;
	struct Program;
	struct Vertices;
	struct Index;
	struct Texture;

	struct Vertex
	{
		struct Vector3 pos;
		struct Vector2 uv;
	};

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

	enum ScaleMode
	{
		SCALE_MODE_ASPECT,
		SCALE_MODE_STEPS,
		SCALE_MODE_FIXED,
		SCALE_MODE_STRETCH
	};

	struct ContextOptions
	{
		char* caption;

		struct Vector2i window_size;
		struct Vector2i window_min_size;
		struct Vector3 clean_color;

		enum ScaleMode scale_mode;
		union {
			struct Vector2i aspect;
			struct Vector2i steps_size;
		};

		bool fullscreen;
	};

	struct Context* ContextCreate(struct ContextOptions options, struct Status* st);
	void ContextDelete(struct Context* context);
	void ContextUpdate(struct Context* context, struct WindowSpecifications* out_window,
	                   struct TimeSpecifications* out_time, struct InputSpecifications* out_input);

	void ContextSetProgram(struct Context* context, const struct Program* program);
	void ContextSetProjection(struct Context* context, struct Matrix4 matrix);
	void ContextSetCamera(struct Context* context, struct Vector3 target, struct Vector3 origin);
	void ContextSetCameraAsMatrix(struct Context* context, struct Matrix4 matrix);

	void ContextDraw(struct Context* context, const struct Vertices* vertices, const struct Index* index,
	                 const struct Texture* color);

	struct Program* ProgramCreate(const char* vertex_code, const char* fragment_code, struct Status* st);
	struct Vertices* VerticesCreate(const struct Vertex* data, size_t length, struct Status* st);
	struct Index* IndexCreate(const uint16_t* data, size_t length, struct Status* st);
	struct Texture* TextureCreate(const struct Image* image, struct Status* st);

	void ProgramDelete(struct Program*);
	void VerticesDelete(struct Vertices*);
	void IndexDelete(struct Index*);
	void TextureDelete(struct Texture*);

#endif
