/*-----------------------------

MIT License

Copyright (c) 2019 Alexander Brandt

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

-------------------------------

 [tiny-gl.c]
 - Alexander Brandt 2019
-----------------------------*/

#include "tiny-gl.h"
#include <math.h>

#ifndef TEST // All this need to be mocked


/*-----------------------------

 ProgramInit()
-----------------------------*/
static inline int sCompileShader(GLuint shader, struct Status* st)
{
	GLint success = GL_FALSE;

	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (success == GL_FALSE)
	{
		StatusSet(st, "ProgramInit", STATUS_ERROR, NULL);
		glGetShaderInfoLog(shader, STATUS_EXPLANATION_LENGTH, NULL, st->explanation);
		return 1;
	}

	return 0;
}

int ProgramInit(struct Program* out, const char* vertex_code, const char* fragment_code, struct Status* st)
{
	GLint success = GL_FALSE;
	GLuint vertex = 0;
	GLuint fragment = 0;

	StatusSet(st, "ProgramInit", STATUS_SUCCESS, NULL);

	// Compile shaders
	if ((vertex = glCreateShader(GL_VERTEX_SHADER)) == 0 || (fragment = glCreateShader(GL_FRAGMENT_SHADER)) == 0)
	{
		StatusSet(st, "ProgramInit", STATUS_ERROR, "Creating GL shader\n");
		goto return_failure;
	}

	glShaderSource(vertex, 1, &vertex_code, NULL);
	glShaderSource(fragment, 1, &fragment_code, NULL);

	if (sCompileShader(vertex, st) != 0 || sCompileShader(fragment, st) != 0)
		goto return_failure;

	// Create program
	if ((out->glptr = glCreateProgram()) == 0)
	{
		StatusSet(st, "ProgramInit", STATUS_ERROR, "Creating GL program\n");
		goto return_failure;
	}

	glAttachShader(out->glptr, vertex);
	glAttachShader(out->glptr, fragment);

	glBindAttribLocation(out->glptr, ATTRIBUTE_POSITION, "vertex_position"); // Before link!
	glBindAttribLocation(out->glptr, ATTRIBUTE_UV, "vertex_uv");

	// Link
	glLinkProgram(out->glptr);
	glGetProgramiv(out->glptr, GL_LINK_STATUS, &success);

	if (success == GL_FALSE)
	{
		StatusSet(st, "ProgramInit", STATUS_ERROR, NULL);
		glGetProgramInfoLog(out->glptr, STATUS_EXPLANATION_LENGTH, NULL, st->explanation);
		goto return_failure;
	}

	// Bye!
	glDeleteShader(vertex); // Set shader to be deleted when glDeleteProgram() happens
	glDeleteShader(fragment);
	return 0;

return_failure:
	if (vertex != 0)
		glDeleteShader(vertex);
	if (fragment != 0)
		glDeleteShader(fragment);
	if (out->glptr != 0)
		glDeleteProgram(out->glptr);

	return 1;
}


/*-----------------------------

 ProgramFree()
-----------------------------*/
inline void ProgramFree(struct Program* program)
{
	glDeleteProgram(program->glptr);
	program->glptr = 0;
}


/*-----------------------------

 VerticesInit()
-----------------------------*/
int VerticesInit(struct Vertices* out, const struct Vertex* data, size_t length, struct Status* st)
{
	GLint reported_size = 0;

	StatusSet(st, "VerticesInit", STATUS_SUCCESS, NULL);

	glGenBuffers(1, &out->glptr);
	glBindBuffer(GL_ARRAY_BUFFER, out->glptr); // Before ask if is!

	if (glIsBuffer(out->glptr) == GL_FALSE)
	{
		StatusSet(st, "VerticesInit", STATUS_ERROR, "Creating GL buffer");
		goto return_failure;
	}

	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(struct Vertex) * length), data, GL_STREAM_DRAW);
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &reported_size);

	if ((size_t)reported_size != (sizeof(struct Vertex) * length))
	{
		StatusSet(st, "VerticesInit", STATUS_ERROR, "Attaching data");
		goto return_failure;
	}

	out->length = length;

	// Bye!
	return 0;

return_failure:
	if (out->glptr != 0)
		glDeleteBuffers(1, &out->glptr);

	return 1;
}


/*-----------------------------

 VerticesFree()
-----------------------------*/
inline void VerticesFree(struct Vertices* vertices)
{
	glDeleteBuffers(1, &vertices->glptr);
	vertices->glptr = 0;
}


/*-----------------------------

 IndexInit()
-----------------------------*/
int IndexInit(struct Index* out, const uint16_t* data, size_t length, struct Status* st)
{
	GLint reported_size = 0;

	StatusSet(st, "IndexInit", STATUS_SUCCESS, NULL);

	glGenBuffers(1, &out->glptr);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out->glptr); // Before ask if is!

	if (glIsBuffer(out->glptr) == GL_FALSE)
	{
		StatusSet(st, "IndexInit", STATUS_ERROR, "Creating GL buffer");
		goto return_failure;
	}

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(sizeof(uint16_t) * length), data, GL_STREAM_DRAW);
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &reported_size);

	if ((size_t)reported_size != (sizeof(uint16_t) * length))
	{
		StatusSet(st, "IndexInit", STATUS_ERROR, "Attaching data");
		goto return_failure;
	}

	out->length = length;

	// Bye!
	return 0;

return_failure:
	if (out->glptr != 0)
		glDeleteBuffers(1, &out->glptr);

	return 1;
}


/*-----------------------------

 IndexFree()
-----------------------------*/
inline void IndexFree(struct Index* index)
{
	glDeleteBuffers(1, &index->glptr);
	index->glptr = 0;
}


/*-----------------------------

 TextureInit()
-----------------------------*/
int TextureInitImage(struct Texture* out, const struct Image* image, enum Filter filter, struct Status* st)
{
	StatusSet(st, "TextureInitImage", STATUS_SUCCESS, NULL);

	if (image->format != IMAGE_RGB8 && image->format != IMAGE_RGBA8 && image->format != IMAGE_GRAY8 &&
	    image->format != IMAGE_GRAYA8)
	{
		StatusSet(st, "TextureInitImage", STATUS_UNEXPECTED_DATA, "Only 8 bits per component images supported");
		return 1;
	}

	glGenTextures(1, &out->glptr);
	glBindTexture(GL_TEXTURE_2D, out->glptr); // Before ask if is!

	if (glIsTexture(out->glptr) == GL_FALSE)
	{
		StatusSet(st, "TextureInitImage", STATUS_ERROR, "Creating GL texture");
		return 1;
	}

	switch (filter)
	{
	case FILTER_BILINEAR:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	case FILTER_TRILINEAR:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;

	case FILTER_PIXEL_BILINEAR:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
	case FILTER_PIXEL_TRILINEAR:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;

	case FILTER_NONE:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
	}

	switch (image->format)
	{
	case IMAGE_RGB8:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)image->width, (GLsizei)image->height, 0, GL_RGB,
		             GL_UNSIGNED_BYTE, image->data);
		break;
	case IMAGE_RGBA8:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)image->width, (GLsizei)image->height, 0, GL_RGBA,
		             GL_UNSIGNED_BYTE, image->data);
		break;
	case IMAGE_GRAY8:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, (GLsizei)image->width, (GLsizei)image->height, 0, GL_LUMINANCE,
		             GL_UNSIGNED_BYTE, image->data);
		break;
	case IMAGE_GRAYA8:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, (GLsizei)image->width, (GLsizei)image->height, 0,
		             GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, image->data);
		break;
	default: break;
	}

	glGenerateMipmap(GL_TEXTURE_2D);
	return 0;
}

int TextureInitFilename(struct Texture* out, const char* image_filename, enum Filter filter, struct Status* st)
{
	struct Image* image = NULL;
	StatusSet(st, "TextureInitFilename", STATUS_SUCCESS, NULL);

	if ((image = ImageLoad(image_filename, st)) == NULL)
		return 1;

	if (TextureInitImage(out, image, filter, st) != 0)
	{
		ImageDelete(image);
		return 1;
	}

	ImageDelete(image);
	return 0;
}


/*-----------------------------

 TextureFree()
-----------------------------*/
inline void TextureFree(struct Texture* texture)
{
	glDeleteTextures(1, &texture->glptr);
	texture->glptr = 0;
}

#endif
