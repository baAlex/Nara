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

#include <stdio.h>
#include "context-private.h"


#ifdef MULTIPLE_CONTEXTS_TEST // A bad idea
static int s_glfw_references = 0;
static struct Context* s_current_context = NULL;
#endif


static inline void sResize(GLFWwindow* window, int new_width, int new_height)
{
	struct Context* context = glfwGetWindowUserPointer(window);
	ViewportResize(context->options.scale_mode, new_width, new_height, context->options.steps_size.x,
				   context->options.steps_size.y);
}


/*-----------------------------

 ContextCreate()
-----------------------------*/
struct Context* ContextCreate(struct ContextOptions options, struct Status* st)
{
	struct Context* context = NULL;
	const GLFWvidmode* vid_mode = NULL;

	StatusSet(st, "ContextCreate", STATUS_SUCCESS, NULL);

	if ((context = calloc(1, sizeof(struct Context))) == NULL)
		return NULL;

	memset(context, 0, sizeof(struct Context));
	memcpy(&context->options, &options, sizeof(struct ContextOptions));

	printf("%s\n", glfwGetVersionString());

	if (glfwInit() != GLFW_TRUE)
	{
		StatusSet(st, "ContextCreate", STATUS_ERROR, "Initialiting GLFW\n");
		goto return_failure;
	}

#ifdef MULTIPLE_CONTEXTS_TEST
	s_glfw_references++;
	s_current_context = context;
#endif

	// Create window
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API); // TODO: Hardcoded
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	if ((context->window = glfwCreateWindow(context->options.window_size.x, context->options.window_size.y,
											context->options.caption, NULL, NULL)) == NULL)
	{
		StatusSet(st, "ContextCreate", STATUS_ERROR, "Creating GLFW window\n");
		goto return_failure;
	}

	glfwMakeContextCurrent(context->window);

	glfwSetWindowSizeLimits(context->window, context->options.windows_min_size.x, context->options.windows_min_size.y,
							GLFW_DONT_CARE, GLFW_DONT_CARE);
	glfwSetWindowUserPointer(context->window, context);

	glfwSetFramebufferSizeCallback(context->window, sResize);
	sResize(context->window, context->options.window_size.x, context->options.window_size.y);

	if (context->options.fullscreen == true)
	{
		vid_mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwSetWindowMonitor(context->window, glfwGetPrimaryMonitor(), 0, 0, vid_mode->width, vid_mode->height,
							 GLFW_DONT_CARE);
	}

	// GL Specific initialization
	glClearColor(context->options.clean_color.x, context->options.clean_color.y, context->options.clean_color.z, 1.0);
	glEnableVertexAttribArray(ATTRIBUTE_POSITION);
	glEnableVertexAttribArray(ATTRIBUTE_NORMAL);
	// glEnableVertexAttribArray(ATTRIBUTE_UV);

	// Bye!
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
void ContextDelete(struct Context* context)
{
	if (context->window != NULL)
		glfwDestroyWindow(context->window);

	free(context);

#ifdef MULTIPLE_CONTEXTS_TEST
	s_glfw_references--;

	if (s_glfw_references == 0)
		glfwTerminate();
#endif
}


/*-----------------------------

 ContextUpdate()
-----------------------------*/
void ContextUpdate(struct Context* context, struct Events* out_events)
{
#ifdef MULTIPLE_CONTEXTS_TEST
	if (s_current_context != context)
	{
		glfwMakeContextCurrent(context->window);
		s_current_context = context;
	}
#else
	(void)context;
#endif

	glfwSwapBuffers(context->window);
	glClear(GL_COLOR_BUFFER_BIT);

	if (out_events != NULL)
	{
		memset(&context->events, 0, sizeof(struct Events));

		glfwPollEvents(); // TODO: It did not goes well with MULTIPLE_CONTEXTS_TEST

		if (glfwWindowShouldClose(context->window) == GLFW_TRUE)
			context->events.system.exit = true;

		memcpy(out_events, &context->events, sizeof(struct Events));
	}
}


/*-----------------------------

 ContextSetProgram()
-----------------------------*/
void ContextSetProgram(struct Context* context, const struct Program* program)
{
#ifdef MULTIPLE_CONTEXTS_TEST
	if (s_current_context != context)
	{
		glfwMakeContextCurrent(context->window);
		s_current_context = context;
	}
#else
	(void)context;
#endif

	if (program != context->current_program)
	{
		context->current_program = program;

		glUseProgram(program->ptr);
		glUniformMatrix4fv(glGetUniformLocation(program->ptr, "projection"), 1, GL_FALSE, context->projection.e);
		glUniformMatrix4fv(glGetUniformLocation(program->ptr, "camera"), 1, GL_FALSE, context->camera.e);
	}
}


/*-----------------------------

 ContextSetProjection()
-----------------------------*/
void ContextSetProjection(struct Context* context, struct Matrix4 matrix)
{
#ifdef MULTIPLE_CONTEXTS_TEST
	if (s_current_context != context)
	{
		glfwMakeContextCurrent(context->window);
		s_current_context = context;
	}
#endif

	memcpy(&context->projection, &matrix, sizeof(struct Matrix4));

	if (context->current_program != NULL)
		glUniformMatrix4fv(glGetUniformLocation(context->current_program->ptr, "projection"), 1, GL_FALSE,
		                   context->projection.e);
}


/*-----------------------------

 ContextSetCamera()
-----------------------------*/
void ContextSetCamera(struct Context* context, struct Matrix4 matrix)
{
#ifdef MULTIPLE_CONTEXTS_TEST
	if (s_current_context != context)
	{
		glfwMakeContextCurrent(context->window);
		s_current_context = context;
	}
#endif

	memcpy(&context->camera, &matrix, sizeof(struct Matrix4));

	if (context->current_program != NULL)
		glUniformMatrix4fv(glGetUniformLocation(context->current_program->ptr, "camera"), 1, GL_FALSE,
		                   context->camera.e);
}


/*-----------------------------

 ContextDraw()
-----------------------------*/
void ContextDraw(struct Context* context, const struct Vertices* vertices, const struct Index* index)
{
#ifdef MULTIPLE_CONTEXTS_TEST
	if (s_current_context != context)
	{
		glfwMakeContextCurrent(context->window);
		s_current_context = context;
	}
#else
	(void)context;
#endif

	glBindBuffer(GL_ARRAY_BUFFER, vertices->ptr);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index->ptr);

	glVertexAttribPointer(ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), NULL);
	glVertexAttribPointer(ATTRIBUTE_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), ((float*)NULL) + 3);

	glDrawElements(GL_LINE_LOOP, index->length, GL_UNSIGNED_SHORT, NULL);
	//glDrawElements(GL_TRIANGLES, index->length, GL_UNSIGNED_SHORT, NULL);
}
