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

#define MIN_WIDTH 320
#define MIN_HEIGHT 240

#define CLEAN_R 0.80f // TODO, a skybox is needed
#define CLEAN_G 0.82f
#define CLEAN_B 0.84f


static void sResizeCallback(GLFWwindow* window, int new_width, int new_height)
{
	struct Context* context = glfwGetWindowUserPointer(window);

	glViewport(0, 0, (GLint)new_width, (GLint)new_height);

	context->window_size.x = new_width;
	context->window_size.y = new_height;
	context->window_resized = true;
}

static void sErrorCallback(int code, const char* description)
{
	printf("[GLFW error] (%i) %s\n", code, description);
}


/*-----------------------------

 ContextCreate()
-----------------------------*/
struct Context* ContextCreate(const struct Options* options, const char* caption, struct Status* st)
{
	struct Context* context = NULL;

	StatusSet(st, "ContextCreate", STATUS_SUCCESS, NULL);
	printf("- Lib-GLFW: %s\n", glfwGetVersionString());

	// Initialization
	if ((context = calloc(1, sizeof(struct Context))) == NULL)
		return NULL;

	if (OptionsRetrieve(options, "r_width", &context->cfg.width, st) != 0 ||
	    OptionsRetrieve(options, "r_height", &context->cfg.height, st) != 0 ||
	    OptionsRetrieve(options, "r_samples", &context->cfg.samples, st) != 0 ||
	    OptionsRetrieve(options, "r_fullscreen", &context->cfg.fullscreen, st) != 0 ||
	    OptionsRetrieve(options, "r_wireframe", &context->cfg.wireframe, st) != 0 ||
	    OptionsRetrieve(options, "r_vsync", &context->cfg.vsync, st) != 0)
		goto return_failure;

	glfwSetErrorCallback(sErrorCallback);

	if (glfwInit() != GLFW_TRUE)
	{
		StatusSet(st, "ContextCreate", STATUS_ERROR, "Initialiting GLFW");
		goto return_failure;
	}

	// Create window-context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_SAMPLES, context->cfg.samples);

	if ((context->window = glfwCreateWindow(context->cfg.width, context->cfg.height, caption, NULL, NULL)) == NULL)
	{
		StatusSet(st, "ContextCreate", STATUS_ERROR, "Creating GLFW window");
		goto return_failure;
	}

	glfwMakeContextCurrent(context->window);

	if (gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress) == 0) // After MakeContext()
	{
		StatusSet(st, "ContextCreate", STATUS_ERROR, "Initialiting GLAD");
		goto return_failure;
	}

	if (context->cfg.vsync == false)
		glfwSwapInterval(0);

	// Callbacks and pretty things
	glfwSetWindowSizeLimits(context->window, MIN_WIDTH, MIN_HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);

	glfwSetWindowUserPointer(context->window, context);

	glfwSetFramebufferSizeCallback(context->window, sResizeCallback);
	glfwSetKeyCallback(context->window, KeyboardCallback);
	glfwSetCursorPosCallback(context->window, MousePositionCallback);
	glfwSetMouseButtonCallback(context->window, MouseClickCallback);

	if (context->cfg.fullscreen == true)
	{
		const GLFWvidmode* vid_mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwSetWindowMonitor(context->window, glfwGetPrimaryMonitor(), 0, 0, vid_mode->width, vid_mode->height,
		                     GLFW_DONT_CARE);
	}

	// OpenGL initialization
	glClearColor(CLEAN_R, CLEAN_G, CLEAN_B, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);

	glDisable(GL_DITHER);

	glEnableVertexAttribArray(ATTRIBUTE_POSITION);
	glEnableVertexAttribArray(ATTRIBUTE_UV);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	// Last things to do
	printf("%s\n", glGetString(GL_VENDOR));
	printf("%s\n", glGetString(GL_RENDERER));
	printf("%s\n", glGetString(GL_VERSION));
	printf("%s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	context->active_gamepad = FindGamedpad();

	context->window_size.x = context->cfg.width;
	context->window_size.y = context->cfg.height;

	sResizeCallback(context->window, context->window_size.x, context->window_size.y);

	// Bye!
	printf("\n");
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

	glfwTerminate();
	free(context);
}


/*-----------------------------

 ContextUpdate()
-----------------------------*/
void ContextUpdate(struct Context* context, struct ContextEvents* out_events)
{
	glfwSwapBuffers(context->window);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	context->window_resized = false;

	glfwPollEvents();

	if (out_events != NULL)
	{
		// Input
		InputStep(context);
		memcpy(out_events, &context->combined, sizeof(struct ContextEvents));

		// Window
		out_events->window_size = context->window_size;
		out_events->resized = context->window_resized;
		out_events->close = (glfwWindowShouldClose(context->window) == GLFW_TRUE) ? true : false;
	}
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
inline void SetProjection(struct Context* context, struct Matrix4 matrix)
{
	memcpy(&context->projection, &matrix, sizeof(struct Matrix4));

	if (context->current_program != NULL)
		glUniformMatrix4fv(context->u_projection, 1, GL_FALSE, &context->projection.e[0][0]);
}


/*-----------------------------

 SetCameraLookAt()
-----------------------------*/
inline void SetCameraLookAt(struct Context* context, struct Vector3 target, struct Vector3 origin)
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

 SetCameraMatrix()
-----------------------------*/
inline void SetCameraMatrix(struct Context* context, struct Matrix4 matrix, struct Vector3 origin)
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
inline void SetHighlight(struct Context* context, struct Vector3 value)
{
	glUniform3fv(context->u_highlight, 1, (float*)&value);
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

 GetWindowSize()
-----------------------------*/
inline struct Vector2i GetWindowSize(const struct Context* context)
{
	return context->window_size;
}
