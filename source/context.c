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


#ifdef MULTIPLE_CONTEXTS_TEST // A bad idea
static int s_glfw_references = 0;
static struct Context* s_current_context = NULL;
#endif


/*-----------------------------

 sKeyboard()
-----------------------------*/
static void sKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// TODO: Everything hardcoded!

	struct Context* context = glfwGetWindowUserPointer(window);
	(void)mods;
	(void)scancode;

	if(action == GLFW_PRESS)
	{
		switch (key)
		{
			case GLFW_KEY_S: context->key_events.a = true; break;
			case GLFW_KEY_D: context->key_events.b = true; break;
			case GLFW_KEY_A: context->key_events.x = true; break;
			case GLFW_KEY_W: context->key_events.y = true; break;

			case GLFW_KEY_Q: context->key_events.lb = true; break;
			case GLFW_KEY_E: context->key_events.rb = true; break;

			case GLFW_KEY_SPACE: context->key_events.view = true; break;
			case GLFW_KEY_ENTER: context->key_events.menu = true; break;
			case GLFW_KEY_F1: context->key_events.guide = true; break;

			case GLFW_KEY_T: context->key_events.ls = true; break;
			case GLFW_KEY_Y: context->key_events.rs = true; break;

			default: break;
		}
	}
	else if(action == GLFW_RELEASE)
	{
		switch (key)
		{
			case GLFW_KEY_S: context->key_events.a = false; break;
			case GLFW_KEY_D: context->key_events.b = false; break;
			case GLFW_KEY_A: context->key_events.x = false; break;
			case GLFW_KEY_W: context->key_events.y = false; break;

			case GLFW_KEY_Q: context->key_events.lb = false; break;
			case GLFW_KEY_E: context->key_events.rb = false; break;

			case GLFW_KEY_SPACE: context->key_events.view = false; break;
			case GLFW_KEY_ENTER: context->key_events.menu = false; break;
			case GLFW_KEY_F1: context->key_events.guide = false; break;

			case GLFW_KEY_T: context->key_events.ls = false; break;
			case GLFW_KEY_Y: context->key_events.rs = false; break;

			default: break;
		}
	}
}


/*-----------------------------

 sResize()
-----------------------------*/
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
	glfwSetKeyCallback(context->window, sKeyboard);

	glfwSwapInterval(0); // TODO: Hardcoded

	if (context->options.fullscreen == true)
	{
		vid_mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwSetWindowMonitor(context->window, glfwGetPrimaryMonitor(), 0, 0, vid_mode->width, vid_mode->height,
							 GLFW_DONT_CARE);
	}

	// Joystick initialization
	context->joystick_id = -1;

	for(int i = GLFW_JOYSTICK_1; i < GLFW_JOYSTICK_LAST; i++)
	{
		if(glfwJoystickPresent(i) == GLFW_TRUE)
		{
			printf("Gamepad found: '%s'\n", glfwGetJoystickName(i));

			if(context->joystick_id == -1)
				context->joystick_id = i;
		}
	}

	// Context specific initialization
	timespec_get(&context->last_update, TIME_UTC);
	timespec_get(&context->one_second_counter, TIME_UTC);

	// GL specific initialization
	ViewportResize(context->options.scale_mode, context->options.window_size.x, context->options.window_size.y,
				   context->options.steps_size.x, context->options.steps_size.y);

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

	// Time calculation
	bool one_second = false;
	double betwen_frames = 0.0;
	struct timespec current_time = {0};
	char str_buffer[64];

	timespec_get(&current_time, TIME_UTC);

	context->frame_no++;

	betwen_frames = current_time.tv_nsec / 1000000.0 + current_time.tv_sec * 1000.0;
	betwen_frames -= context->last_update.tv_nsec / 1000000.0 + context->last_update.tv_sec * 1000.0;

	if (current_time.tv_sec > context->one_second_counter.tv_sec)
	{
		context->one_second_counter = current_time;
		one_second = true;

		snprintf(str_buffer, 64, "%s | %li fps (~%.2f ms)", context->options.caption,
				context->frame_no - context->fps_counter, betwen_frames);
		glfwSetWindowTitle(context->window, str_buffer);

		context->fps_counter = context->frame_no;
	}

	// Events
	const uint8_t* joy_buttons = NULL;
	int joy_buttons_no = 0;

	if (out_events != NULL)
	{
		glfwPollEvents(); // TODO: This did not goes well with MULTIPLE_CONTEXTS_TEST
		joy_buttons = glfwGetJoystickButtons(context->joystick_id, &joy_buttons_no); // no requires PollEvents()

		out_events->a = (context->key_events.a == true || (joy_buttons_no >= 1 && joy_buttons[0] == GLFW_TRUE)) ? true : false;
		out_events->b = (context->key_events.b == true || (joy_buttons_no >= 2 && joy_buttons[1] == GLFW_TRUE)) ? true : false;
		out_events->x = (context->key_events.x == true || (joy_buttons_no >= 3 && joy_buttons[2] == GLFW_TRUE)) ? true : false;
		out_events->y = (context->key_events.y == true || (joy_buttons_no >= 4 && joy_buttons[3] == GLFW_TRUE)) ? true : false;

		out_events->lb = (context->key_events.lb == true || (joy_buttons_no >= 5 && joy_buttons[4] == GLFW_TRUE)) ? true : false;
		out_events->rb = (context->key_events.rb == true || (joy_buttons_no >= 6 && joy_buttons[5] == GLFW_TRUE)) ? true : false;

		out_events->view = (context->key_events.view == true || (joy_buttons_no >= 7 && joy_buttons[6] == GLFW_TRUE)) ? true : false;
		out_events->menu = (context->key_events.menu == true || (joy_buttons_no >= 8 && joy_buttons[7] == GLFW_TRUE)) ? true : false;
		out_events->guide = (context->key_events.guide == true || (joy_buttons_no >= 9 && joy_buttons[8] == GLFW_TRUE)) ? true : false;

		out_events->ls = (context->key_events.ls == true || (joy_buttons_no >= 5 && joy_buttons[9] == GLFW_TRUE)) ? true : false;
		out_events->rs = (context->key_events.rs == true || (joy_buttons_no >= 6 && joy_buttons[10] == GLFW_TRUE)) ? true : false;

		// Misc
		out_events->system.exit = (glfwWindowShouldClose(context->window) == GLFW_TRUE) ? true : false;
		out_events->time.frame_no = context->frame_no;
		out_events->time.one_second = one_second;
		out_events->time.betwen_frames = betwen_frames;
	}

	context->last_update = current_time;
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
		glUniform3fv(glGetUniformLocation(context->current_program->ptr, "camera"), 2, (float*)&context->camera);
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
		                   context->projection.e); // TODO: This Uniform() is necessary?
}


/*-----------------------------

 ContextSetCamera()
-----------------------------*/
void ContextSetCamera(struct Context* context, struct Vector3 target, struct Vector3 origin)
{
#ifdef MULTIPLE_CONTEXTS_TEST
	if (s_current_context != context)
	{
		glfwMakeContextCurrent(context->window);
		s_current_context = context;
	}
#endif

	context->camera[0] = target;
	context->camera[1] = origin;

	if (context->current_program != NULL)
		glUniform3fv(glGetUniformLocation(context->current_program->ptr, "camera"), 2, (float*)&context->camera);
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
	// glDrawElements(GL_TRIANGLES, index->length, GL_UNSIGNED_SHORT, NULL);
}
