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

 [context-gl.c]
 - Alexander Brandt 2019
-----------------------------*/

#include <math.h>
#include <stdlib.h>

#include "context-gl.h"


/*-----------------------------

 ProgramCreate()
-----------------------------*/
static inline int sCompileShader(GLuint shader, struct Status* st)
{
	GLint success = GL_FALSE;

	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (success == GL_FALSE)
	{
		StatusSet(st, "ProgramCreate", STATUS_ERROR, NULL);
		glGetShaderInfoLog(shader, STATUS_EXPLANATION_LENGTH, NULL, st->explanation);
		return 1;
	}

	return 0;
}

struct Program* ProgramCreate(const char* vertex_code, const char* fragment_code, struct Status* st)
{
	struct Program* program = NULL;
	GLint success = GL_FALSE;
	GLuint vertex = 0;
	GLuint fragment = 0;

	StatusSet(st, "ProgramCreate", STATUS_SUCCESS, NULL);

	if ((program = malloc(sizeof(struct Program))) == NULL)
		StatusSet(st, "Program", STATUS_MEMORY_ERROR, NULL);
	else
	{
		// Compile shaders
		if ((vertex = glCreateShader(GL_VERTEX_SHADER)) == 0 || (fragment = glCreateShader(GL_FRAGMENT_SHADER)) == 0)
		{
			StatusSet(st, "ProgramCreate", STATUS_ERROR, "Creating GL shader\n");
			goto return_failure;
		}

		glShaderSource(vertex, 1, &vertex_code, NULL);
		glShaderSource(fragment, 1, &fragment_code, NULL);

		if (sCompileShader(vertex, st) != 0 || sCompileShader(fragment, st) != 0)
			goto return_failure;

		// Create program
		if ((program->ptr = glCreateProgram()) == 0)
		{
			StatusSet(st, "ProgramCreate", STATUS_ERROR, "Creating GL program\n");
			goto return_failure;
		}

		glAttachShader(program->ptr, vertex);
		glAttachShader(program->ptr, fragment);

		glBindAttribLocation(program->ptr, ATTRIBUTE_POSITION, "vertex_position"); // Before link!
		glBindAttribLocation(program->ptr, ATTRIBUTE_UV, "vertex_uv");

		// Link
		glLinkProgram(program->ptr);
		glGetProgramiv(program->ptr, GL_LINK_STATUS, &success);

		if (success == GL_FALSE)
		{
			StatusSet(st, "ProgramCreate", STATUS_ERROR, NULL);
			glGetProgramInfoLog(program->ptr, STATUS_EXPLANATION_LENGTH, NULL, st->explanation);
			goto return_failure;
		}
	}

	// Bye!
	glDeleteShader(vertex); // Set shader to be deleted when glDeleteProgram() happens
	glDeleteShader(fragment);
	return program;

return_failure:
	if (program != NULL)
	{
		if (vertex != 0)
			glDeleteShader(vertex);
		if (fragment != 0)
			glDeleteShader(fragment);
		if (program->ptr != 0)
			glDeleteProgram(program->ptr);

		free(program);
	}

	return NULL;
}


/*-----------------------------

 ProgramDelete()
-----------------------------*/
inline void ProgramDelete(struct Program* program)
{
	glDeleteProgram(program->ptr);
	free(program);
}


/*-----------------------------

 VerticesCreate()
-----------------------------*/
struct Vertices* VerticesCreate(const struct Vertex* data, size_t length, struct Status* st)
{
	struct Vertices* vertices = NULL;
	GLint reported_size = 0;

	StatusSet(st, "VerticesCreate", STATUS_SUCCESS, NULL);

	if ((vertices = calloc(1, sizeof(struct Vertices))) == NULL)
		StatusSet(st, "VerticesCreate", STATUS_MEMORY_ERROR, NULL);
	else
	{
		glGenBuffers(1, &vertices->ptr);
		glBindBuffer(GL_ARRAY_BUFFER, vertices->ptr); // Before ask if is!

		if (glIsBuffer(vertices->ptr) == GL_FALSE)
		{
			StatusSet(st, "VerticesCreate", STATUS_ERROR, "Creating GL buffer");
			goto return_failure;
		}

		glBufferData(GL_ARRAY_BUFFER, sizeof(struct Vertex) * length, data, GL_STREAM_DRAW);
		glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &reported_size);

		if ((size_t)reported_size != (sizeof(struct Vertex) * length))
		{
			StatusSet(st, "VerticesCreate", STATUS_ERROR, "Attaching data");
			goto return_failure;
		}

		vertices->length = length;
	}

	// Bye!
	return vertices;

return_failure:
	if (vertices != NULL)
	{
		if (vertices->ptr != 0)
			glDeleteBuffers(1, &vertices->ptr);

		free(vertices);
	}

	return NULL;
}


/*-----------------------------

 VerticesDelete()
-----------------------------*/
inline void VerticesDelete(struct Vertices* vertices)
{
	glDeleteBuffers(1, &vertices->ptr);
	free(vertices);
}


/*-----------------------------

 IndexCreate()
-----------------------------*/
struct Index* IndexCreate(const uint16_t* data, size_t length, struct Status* st)
{
	struct Index* index = NULL;
	GLint reported_size = 0;

	StatusSet(st, "IndexCreate", STATUS_SUCCESS, NULL);

	if ((index = calloc(1, sizeof(struct Index))) == NULL)
		StatusSet(st, "IndexCreate", STATUS_MEMORY_ERROR, NULL);
	else
	{
		glGenBuffers(1, &index->ptr);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index->ptr); // Before ask if is!

		if (glIsBuffer(index->ptr) == GL_FALSE)
		{
			StatusSet(st, "IndexCreate", STATUS_ERROR, "Creating GL buffer");
			goto return_failure;
		}

		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * length, data, GL_STREAM_DRAW);
		glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &reported_size);

		if ((size_t)reported_size != (sizeof(uint16_t) * length))
		{
			StatusSet(st, "IndexCreate", STATUS_ERROR, "Attaching data");
			goto return_failure;
		}

		index->length = length;
	}

	// Bye!
	return index;

return_failure:
	if (index != NULL)
	{
		if (index->ptr != 0)
			glDeleteBuffers(1, &index->ptr);

		free(index);
	}

	return NULL;
}


/*-----------------------------

 IndexDelete()
-----------------------------*/
inline void IndexDelete(struct Index* index)
{
	glDeleteBuffers(1, &index->ptr);
	free(index);
}


/*-----------------------------

 TextureCreate()
-----------------------------*/
struct Texture* TextureCreate(const struct Image* image, struct Status* st)
{
	struct Texture* texture = NULL;

	StatusSet(st, "TextureCreate", STATUS_SUCCESS, NULL);

	if (image->format != IMAGE_RGB8)
	{
		StatusSet(st, "TextureCreate", STATUS_UNEXPECTED_DATA, "Only RGB8 images supported");
		return NULL;
	}

	if ((texture = calloc(1, sizeof(struct Texture))) == NULL)
		StatusSet(st, "TextureCreate", STATUS_MEMORY_ERROR, NULL);
	else
	{
		glGenTextures(1, &texture->ptr);
		glBindTexture(GL_TEXTURE_2D, texture->ptr); // Before ask if is!

		if (glIsTexture(texture->ptr) == GL_FALSE)
		{
			StatusSet(st, "TextureCreate", STATUS_ERROR, "Creating GL texture");
			free(texture);
			return NULL;
		}

		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // TODO: a texture property?
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->width, image->height, 0, GL_RGB, GL_UNSIGNED_BYTE, image->data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	return texture;
}


/*-----------------------------

 TextureDelete()
-----------------------------*/
inline void TextureDelete(struct Texture* texture)
{
	glDeleteTextures(1, &texture->ptr);
	free(texture);
}


/*-----------------------------

 ViewportResize()
-----------------------------*/
void ViewportResize(enum ScaleMode mode, int new_w, int new_h, int aspect_w, int aspect_h)
{
	GLint final_size_x = 0;
	GLint final_size_y = 0;
	GLint final_position_x = 0;
	GLint final_position_y = 0;

	// Scaling keeping the aspect ration
	if (mode == SCALE_MODE_ASPECT)
	{
		float new_aspect = (float)new_w / (float)new_h;
		float old_aspect = (float)aspect_w / (float)aspect_h;

		if (new_aspect < old_aspect)
		{
			float h = (float)new_w / old_aspect;

			final_position_y = (new_h / 2 - h / 2);
			final_size_x = new_w;
			final_size_y = (int)(ceil(h));
		}
		else
		{
			float w = (float)new_h * old_aspect;

			final_position_x = (new_w / 2 - w / 2);
			final_size_x = (int)(ceil(w));
			final_size_y = new_h;
		}
	}

	// Scaling in steps of two
	else if (mode == SCALE_MODE_STEPS)
	{
		float scale_x = (new_w + 2) / aspect_w; // 2 = error margin
		float scale_y = (new_h + 2) / aspect_h;

		if (scale_x != 0 && scale_y != 0)
		{
			if (scale_x < scale_y)
			{
				final_position_x = new_w / 2 - (aspect_w * scale_x) / 2;
				final_position_y = new_h / 2 - (aspect_h * scale_x) / 2;
				final_size_x = aspect_w * scale_x;
				final_size_y = aspect_h * scale_x;
			}
			else
			{
				final_position_x = new_w / 2 - (aspect_w * scale_y) / 2;
				final_position_y = new_h / 2 - (aspect_h * scale_y) / 2;
				final_size_x = aspect_w * scale_y;
				final_size_y = aspect_h * scale_y;
			}
		}
		else
		{
			final_size_x = aspect_w;
			final_size_y = aspect_h;
		}
	}

	// No scaling
	else if (mode == SCALE_MODE_FIXED)
	{
		final_position_x = (new_w / 2 - aspect_w / 2);
		final_position_y = (new_h / 2 - aspect_h / 2);
		final_size_x = aspect_w;
		final_size_y = aspect_h;
	}

	// Stretch
	else if (mode == SCALE_MODE_STRETCH)
	{
		final_size_x = new_w;
		final_size_y = new_h;
	}

	glViewport(final_position_x, final_position_y, final_size_x, final_size_y);
}
