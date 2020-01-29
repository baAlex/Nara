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
 - Alexander Brandt 2019-2020
-----------------------------*/

#include "private.h"

#define MIN_WIDTH 320
#define MIN_HEIGHT 240

#define CLEAN_R 0.82f // TODO, a skybox is needed
#define CLEAN_G 0.85f
#define CLEAN_B 0.87f


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
struct Context* ContextCreate(const struct jaConfiguration* config, const char* caption, struct jaStatus* st)
{
	struct Context* context = NULL;

	jaStatusSet(st, "ContextCreate", STATUS_SUCCESS, NULL);
	printf("- Lib-GLFW: %s\n", glfwGetVersionString());

	// Initialization
	{
		const char* filter = NULL;

		if ((context = calloc(1, sizeof(struct Context))) == NULL)
			return NULL;

		if (jaCvarValue(jaCvarFind(config, "render.width"), &context->cfg.width, st) != 0 ||
		    jaCvarValue(jaCvarFind(config, "render.height"), &context->cfg.height, st) != 0 ||
		    jaCvarValue(jaCvarFind(config, "render.samples"), &context->cfg.samples, st) != 0 ||
		    jaCvarValue(jaCvarFind(config, "render.fullscreen"), &context->cfg.fullscreen, st) != 0 ||
		    jaCvarValue(jaCvarFind(config, "render.wireframe"), &context->cfg.wireframe, st) != 0 ||
		    jaCvarValue(jaCvarFind(config, "render.vsync"), &context->cfg.vsync, st) != 0 ||
		    jaCvarValue(jaCvarFind(config, "render.filter"), &filter, st) != 0)
			goto return_failure;

		if (strcmp(filter, "pixel") == 0)
			context->cfg.filter = FILTER_NONE;
		else if (strcmp(filter, "bilinear") == 0)
			context->cfg.filter = FILTER_BILINEAR;
		else if (strcmp(filter, "trilinear") == 0)
			context->cfg.filter = FILTER_TRILINEAR;
		else if (strcmp(filter, "pixel_bilinear") == 0)
			context->cfg.filter = FILTER_PIXEL_BILINEAR;
		else if (strcmp(filter, "pixel_trilinear") == 0)
			context->cfg.filter = FILTER_PIXEL_TRILINEAR;
	}

	// GLFW context
	glfwSetErrorCallback(sErrorCallback);

	if (glfwInit() != GLFW_TRUE)
	{
		jaStatusSet(st, "ContextCreate", STATUS_ERROR, "Initialiting GLFW");
		goto return_failure;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_SAMPLES, context->cfg.samples);

	if ((context->window = glfwCreateWindow(context->cfg.width, context->cfg.height, caption, NULL, NULL)) == NULL)
	{
		jaStatusSet(st, "ContextCreate", STATUS_ERROR, "Creating GLFW window");
		goto return_failure;
	}

	glfwMakeContextCurrent(context->window); // As soon CreateWindow() is called

	glfwSetWindowSizeLimits(context->window, MIN_WIDTH, MIN_HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);
	glfwSetWindowUserPointer(context->window, context);

	glfwSetFramebufferSizeCallback(context->window, sResizeCallback);
	glfwSetKeyCallback(context->window, KeyboardCallback);

	if (context->cfg.vsync == false)
		glfwSwapInterval(0);

	// GLAD loader
	if (gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress) == 0) // After MakeContext()
	{
		jaStatusSet(st, "ContextCreate", STATUS_ERROR, "Initialiting GLAD");
		goto return_failure;
	}

	// Fullscreen
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
	if (context == NULL)
		return;

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
	context->window_resized = false;

	glfwPollEvents();

	// Flip buffers *after* process the inputs callbacks, so
	// if these takes time at least the screen didn't goes black
	glfwSwapBuffers(context->window);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
