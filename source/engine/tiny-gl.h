/*-----------------------------

 [tiny-gl.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef TINY_GL_H
#define TINY_GL_H

	#include "image.h"
	#include "status.h"
	#include "vector.h"

	#ifndef TEST
	#define GLFW_INCLUDE_ES2
	#include <GLFW/glfw3.h>
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
