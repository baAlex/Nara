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

struct Context
{
	GLFWwindow* window;
	struct Vector2i window_size;
	bool window_resized;

	struct ContextOptions options;

	// Modules:
	struct ContextOpenGl opengl;
	struct ContextInput input;
	struct ContextTime time;
};


inline void ContextSetProgram(struct Context* context, const struct Program* program)
{
	SetProgram(&context->opengl, program);
}

inline void ContextSetProjection(struct Context* context, struct Matrix4 matrix)
{
	SetProjection(&context->opengl, matrix);
}

inline void ContextSetCamera(struct Context* context, struct Vector3 target, struct Vector3 origin)
{
	SetCamera(&context->opengl, target, origin);
}

inline void ContextSetCameraAsMatrix(struct Context* context, struct Matrix4 matrix, struct Vector3 origin)
{
	SetCameraAsMatrix(&context->opengl, matrix, origin);
}

void ContextDraw(struct Context* context, const struct Vertices* vertices, const struct Index* index,
                 const struct Texture* color)
{
	(void)context;
	Draw(vertices, index, color);
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

	ViewportResize(context->options.scale_mode, new_width, new_height, context->options.steps_size.x,
	               context->options.steps_size.y);

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
	ContextOpenGlInitialization(&context->opengl, options.clean_color);

	context->window_size = context->options.window_size;
	ViewportResize(context->options.scale_mode, context->options.window_size.x, context->options.window_size.y,
	               context->options.steps_size.x, context->options.steps_size.y);

	// Bye!
	printf("%s\n", glGetString(GL_VENDOR));
	printf("%s\n", glGetString(GL_RENDERER));
	printf("%s\n", glGetString(GL_VERSION));
	printf("%s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("%s\n", glGetString(GL_EXTENSIONS));

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
	ContextOpenGlStep(&context->opengl);

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
