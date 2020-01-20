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

 [state.c]
 - Alexander Brandt 2019-2020
-----------------------------*/

#include "private.h"


/*-----------------------------

 SetFunctionKeyCallback()
-----------------------------*/
inline void SetFunctionKeyCallback(struct Context* context, int key,
                                   void (*callback)(const struct Context*, const struct ContextEvents*, bool press))
{
	context->fcallback[key - 1] = callback;
}


/*-----------------------------

 SetTitle()
-----------------------------*/
inline void SetTitle(struct Context* context, const char* title)
{
	glfwSetWindowTitle(context->window, title);
}


/*-----------------------------

 SetProgram()
-----------------------------*/
inline void SetProgram(struct Context* context, const struct Program* program)
{
	if (program != context->current_program)
	{
		context->current_program = program;
		context->u_projection = glGetUniformLocation(program->glptr, "projection");
		context->u_camera_projection = glGetUniformLocation(program->glptr, "camera_projection");
		context->u_camera_origin = glGetUniformLocation(program->glptr, "camera_origin");
		context->u_highlight = glGetUniformLocation(program->glptr, "highlight");

		context->u_texture[0] = glGetUniformLocation(program->glptr, "texture0");
		context->u_texture[1] = glGetUniformLocation(program->glptr, "texture1");
		context->u_texture[2] = glGetUniformLocation(program->glptr, "texture2");
		context->u_texture[3] = glGetUniformLocation(program->glptr, "texture3");
		context->u_texture[4] = glGetUniformLocation(program->glptr, "texture4");
		context->u_texture[5] = glGetUniformLocation(program->glptr, "texture5");
		context->u_texture[6] = glGetUniformLocation(program->glptr, "texture6");
		context->u_texture[7] = glGetUniformLocation(program->glptr, "texture7");

		glUseProgram(program->glptr);

		glUniformMatrix4fv(context->u_projection, 1, GL_FALSE, &context->projection.e[0][0]);
		glUniformMatrix4fv(context->u_camera_projection, 1, GL_FALSE, &context->camera.e[0][0]);
		glUniform3fv(context->u_camera_origin, 1, (float*)&context->camera_origin);
		glUniform1i(context->u_texture[0], 0);
		glUniform1i(context->u_texture[1], 1);
		glUniform1i(context->u_texture[2], 2);
		glUniform1i(context->u_texture[3], 3);
		glUniform1i(context->u_texture[4], 4);
		glUniform1i(context->u_texture[5], 5);
		glUniform1i(context->u_texture[6], 6);
		glUniform1i(context->u_texture[7], 7);
	}
}


/*-----------------------------

 SetVertices()
-----------------------------*/
inline void SetVertices(struct Context* context, const struct Vertices* vertices)
{
	if (vertices != context->current_vertices)
	{
		context->current_vertices = vertices;

		glBindBuffer(GL_ARRAY_BUFFER, vertices->glptr);

		glVertexAttribPointer(ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), NULL);
		glVertexAttribPointer(ATTRIBUTE_UV, 2, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), ((float*)NULL) + 3);
	}
}


/*-----------------------------

 SetTexture()
-----------------------------*/
inline void SetTexture(struct Context* context, int unit, const struct Texture* texture)
{
	(void)context;
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, texture->glptr);
}


/*-----------------------------

 SetProjection()
-----------------------------*/
inline void SetProjection(struct Context* context, struct jaMatrix4 matrix)
{
	memcpy(&context->projection, &matrix, sizeof(struct jaMatrix4));

	if (context->current_program != NULL)
		glUniformMatrix4fv(context->u_projection, 1, GL_FALSE, &context->projection.e[0][0]);
}


/*-----------------------------

 SetCameraLookAt()
-----------------------------*/
inline void SetCameraLookAt(struct Context* context, struct jaVector3 target, struct jaVector3 origin)
{
	context->camera_origin = origin;
	context->camera = jaMatrix4LookAt(origin, target, (struct jaVector3){0.0, 0.0, 1.0});

	if (context->current_program != NULL)
	{
		glUniformMatrix4fv(context->u_camera_projection, 1, GL_FALSE, &context->camera.e[0][0]);
		glUniform3fv(context->u_camera_origin, 1, (float*)&context->camera_origin);
	}
}


/*-----------------------------

 SetCameraMatrix()
-----------------------------*/
inline void SetCameraMatrix(struct Context* context, struct jaMatrix4 matrix, struct jaVector3 origin)
{
	context->camera_origin = origin;
	context->camera = matrix;

	if (context->current_program != NULL)
	{
		glUniformMatrix4fv(context->u_camera_projection, 1, GL_FALSE, &context->camera.e[0][0]);
		glUniform3fv(context->u_camera_origin, 1, (float*)&context->camera_origin);
	}
}


/*-----------------------------

 SetHighlight()
-----------------------------*/
inline void SetHighlight(struct Context* context, struct jaVector3 value)
{
	glUniform3fv(context->u_highlight, 1, (float*)&value);
}


/*-----------------------------

 GetWindowSize()
-----------------------------*/
inline struct jaVector2i GetWindowSize(const struct Context* context)
{
	return context->window_size;
}


/*-----------------------------

 Draw()
-----------------------------*/
inline void Draw(struct Context* context, const struct Index* index)
{
	(void)context;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index->glptr);
	glDrawElements((context->cfg.wireframe == false) ? GL_TRIANGLES : GL_LINES, (GLsizei)index->length,
	               GL_UNSIGNED_SHORT, NULL);
}


/*-----------------------------

 TakeScreenshot()
-----------------------------*/
int TakeScreenshot(const struct Context* context, const char* filename, struct jaStatus* st)
{
	struct jaImage* image = NULL;
	GLenum error;

	if ((image = jaImageCreate(IMAGE_RGBA8, (size_t)context->window_size.x, (size_t)context->window_size.y)) == NULL)
	{
		jaStatusSet(st, "TakeScreenshot", STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	glReadPixels(0, 0, context->window_size.x, context->window_size.y, GL_RGBA, GL_UNSIGNED_BYTE, image->data);

	if ((error = glGetError()) != GL_NO_ERROR)
	{
		// TODO, glReadPixels has tons of corners where it can fail.
		jaStatusSet(st, "TakeScreenshot", STATUS_ERROR, NULL);
		goto return_failure;
	}

	if ((jaImageSaveSgi(image, filename, st)) != 0)
		goto return_failure;

	jaImageDelete(image);
	return 0;

return_failure:
	if (image != NULL)
		jaImageDelete(image);

	return 1;
}
