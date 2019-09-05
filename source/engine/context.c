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

 [context.c]
 - Alexander Brandt 2019
-----------------------------*/

#include "context-private.h"
#include <stdio.h>

#ifndef TINY_GL_H
#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#endif

struct Context
{
	GLFWwindow* window;
	struct Vector2i window_size;
	bool window_resized;

	struct ContextOptions options;

	struct Matrix4 projection;
	struct Matrix4 camera;
	struct Vector3 camera_origin;

	const struct Program* current_program;
	const struct Texture* current_diffuse;

	GLint u_projection;        // For current program
	GLint u_camera_projection; // "
	GLint u_camera_origin;     // "
	GLint u_color_texture;     // "

	// Modules:
	struct ContextInput input;
	struct ContextTime time;
};


/*-----------------------------

 ContextSetProgram()
-----------------------------*/
void ContextSetProgram(struct Context* context, const struct Program* program)
{
	if (program != context->current_program)
	{
		context->current_program = program;
		context->u_projection = glGetUniformLocation(program->glptr, "projection");
		context->u_camera_projection = glGetUniformLocation(context->current_program->glptr, "camera_projection");
		context->u_camera_origin = glGetUniformLocation(context->current_program->glptr, "camera_origin");
		context->u_color_texture = glGetUniformLocation(context->current_program->glptr, "color_texture");

		glUseProgram(program->glptr);

		glUniformMatrix4fv(context->u_projection, 1, GL_FALSE, &context->projection.e[0][0]);
		glUniformMatrix4fv(context->u_camera_projection, 1, GL_FALSE, &context->camera.e[0][0]);
		glUniform3fv(context->u_camera_origin, 1, (float*)&context->camera_origin);
		glUniform1i(context->u_color_texture, 0); // Texture unit 0
	}
}


/*-----------------------------

 ContextSetProjection()
-----------------------------*/
inline void ContextSetProjection(struct Context* context, struct Matrix4 matrix)
{
	memcpy(&context->projection, &matrix, sizeof(struct Matrix4));

	if (context->current_program != NULL)
		glUniformMatrix4fv(context->u_projection, 1, GL_FALSE, &context->projection.e[0][0]);
}


/*-----------------------------

 ContextSetDiffuse()
-----------------------------*/
inline void ContextSetDiffuse(struct Context* context, const struct Texture* diffuse)
{
	if (diffuse != context->current_diffuse)
	{
		context->current_diffuse = diffuse;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, context->current_diffuse->glptr);
	}
}


/*-----------------------------

 ContextSetCameraLookAt()
-----------------------------*/
inline void ContextSetCameraLookAt(struct Context* context, struct Vector3 target, struct Vector3 origin)
{
	context->camera_origin = origin;
	context->camera = Matrix4LookAt(origin, target, (struct Vector3){0.0, 0.0, 1.0});

	if (context->current_program != NULL)
	{
		glUniformMatrix4fv(context->u_camera_projection, 1, GL_FALSE, &context->camera.e[0][0]);
		glUniform3fv(context->u_camera_origin, 1, (float*)&context->camera_origin);
	}
}


/*-----------------------------

 ContextSetCameraMatrix()
-----------------------------*/
inline void ContextSetCameraMatrix(struct Context* context, struct Matrix4 matrix, struct Vector3 origin)
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

 ContextDraw()
-----------------------------*/
void ContextDraw(struct Context* context, const struct Vertices* vertices, const struct Index* index)
{
	(void)context;

	glBindBuffer(GL_ARRAY_BUFFER, vertices->glptr);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index->glptr);

	glVertexAttribPointer(ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), NULL);
	glVertexAttribPointer(ATTRIBUTE_UV, 2, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), ((float*)NULL) + 3);

	// glDrawElements(GL_LINES, index->length, GL_UNSIGNED_SHORT, NULL);
	glDrawElements(GL_TRIANGLES, index->length, GL_UNSIGNED_SHORT, NULL);
}


/*-----------------------------

 sKeyboardCallback()
-----------------------------*/
static void sKeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	struct Context* context = glfwGetWindowUserPointer(window);
	(void)scancode;
	(void)mods;

	ReceiveKeyboardKey(&context->input, key, action);
}


/*-----------------------------

 sResizeCallback()
-----------------------------*/
static inline void sResizeCallback(GLFWwindow* window, int new_width, int new_height)
{
	struct Context* context = glfwGetWindowUserPointer(window);

	glViewport(0, 0, (GLint)new_width, (GLint)new_height);

	context->window_size.x = new_width;
	context->window_size.y = new_height;
	context->window_resized = true;
}


/*-----------------------------

 ContextCreate()
-----------------------------*/
struct Context* ContextCreate(struct ContextOptions options, struct Status* st)
{
	struct Context* context = NULL;
	const GLFWvidmode* vid_mode = NULL;

	StatusSet(st, "ContextCreate", STATUS_SUCCESS, NULL);

	// Object/Initialization
	if ((context = calloc(1, sizeof(struct Context))) == NULL)
		return NULL;

	memcpy(&context->options, &options, sizeof(struct ContextOptions));

	printf("%s\n", glfwGetVersionString());

	if (glfwInit() != GLFW_TRUE)
	{
		StatusSet(st, "ContextCreate", STATUS_ERROR, "Initialiting GLFW");
		goto return_failure;
	}

	// Create window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 2);

	if ((context->window =
	         glfwCreateWindow(options.window_size.x, options.window_size.y, options.caption, NULL, NULL)) == NULL)
	{
		StatusSet(st, "ContextCreate", STATUS_ERROR, "Creating GLFW window");
		goto return_failure;
	}

	glfwMakeContextCurrent(context->window);
	glfwSwapInterval(0);

	glfwSetWindowSizeLimits(context->window, options.window_min_size.x, options.window_min_size.y, GLFW_DONT_CARE,
	                        GLFW_DONT_CARE);

	glfwSetWindowUserPointer(context->window, context);
	glfwSetFramebufferSizeCallback(context->window, sResizeCallback);
	glfwSetKeyCallback(context->window, sKeyboardCallback);

	if (context->options.fullscreen == true)
	{
		vid_mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwSetWindowMonitor(context->window, glfwGetPrimaryMonitor(), 0, 0, vid_mode->width, vid_mode->height,
		                     GLFW_DONT_CARE);
	}

	// Modules initialization
	ContextInputInitialization(&context->input);
	ContextTimeInitialization(&context->time);

	// OpenGL initialization
	glClearColor(options.clean_color.x, options.clean_color.y, options.clean_color.z, 1.0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);

	glDisable(GL_DITHER);

	glEnableVertexAttribArray(ATTRIBUTE_POSITION);
	glEnableVertexAttribArray(ATTRIBUTE_UV);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	// Bye!
	context->window_size = context->options.window_size;
	sResizeCallback(context->window, context->options.window_size.x, context->options.window_size.y);

	printf("%s\n", glGetString(GL_VENDOR));
	printf("%s\n", glGetString(GL_RENDERER));
	printf("%s\n", glGetString(GL_VERSION));
	printf("%s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	return context;

return_failure:
	ContextDelete(context);
	return NULL;
}


/*-----------------------------

 ContextDelete()
-----------------------------*/
inline void ContextDelete(struct Context* context)
{
	if (context->window != NULL)
		glfwDestroyWindow(context->window);

	free(context);
}


/*-----------------------------

 ContextUpdate()
-----------------------------*/
void ContextUpdate(struct Context* context, struct WindowSpecifications* out_window,
                   struct TimeSpecifications* out_time, struct InputSpecifications* out_input)
{
	glfwSwapBuffers(context->window);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Window module abstraction (the context is the window)
	{
		context->window_resized = false;
		glfwPollEvents();

		if (out_window != NULL)
		{
			out_window->size = context->window_size;
			out_window->resized = context->window_resized;
			out_window->close = (glfwWindowShouldClose(context->window) == GLFW_TRUE) ? true : false;
		}
	}

	// Time module
	if (out_time != NULL)
	{
		ContextTimeStep(&context->time);
		memcpy(out_time, &context->time.specs, sizeof(struct TimeSpecifications));

		if (context->time.specs.one_second == true)
		{
			char buffer[64];

			snprintf(buffer, 64, "%s | %i fps (~%.2f ms)", context->options.caption,
			         context->time.specs.frames_per_second, context->time.specs.miliseconds_betwen);
			glfwSetWindowTitle(context->window, buffer);
		}
	}

	// Input module (requires glfwPollEvents())
	if (out_input != NULL)
	{
		ContextInputStep(&context->input);
		memcpy(out_input, &context->input.specs, sizeof(struct InputSpecifications));
	}
}
