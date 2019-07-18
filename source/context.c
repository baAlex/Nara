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


/*-----------------------------

 sKeyboardCallback()
-----------------------------*/
static void sKeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	struct Context* context = glfwGetWindowUserPointer(window);
	(void)mods;
	(void)scancode;

	switch (key)
	{
	case GLFW_KEY_S: context->events.a = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_D: context->events.b = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_A: context->events.x = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_W: context->events.y = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_Q: context->events.lb = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_E: context->events.rb = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_SPACE: context->events.view = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_ENTER: context->events.menu = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_F1: context->events.guide = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_T: context->events.ls = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_Y: context->events.rs = (action == GLFW_PRESS) ? true : false; break;

	default: break;
	}
}


/*-----------------------------

 sResizeCallback()
-----------------------------*/
static inline void sResizeCallback(GLFWwindow* window, int new_width, int new_height)
{
	struct Context* context = glfwGetWindowUserPointer(window);

	ViewportResize(context->options.scale_mode, new_width, new_height, context->options.steps_size.x,
	               context->options.steps_size.y);

	context->events.window.resize = true;
	context->events.window.width = new_width;
	context->events.window.height = new_height;
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
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	glfwWindowHint(GLFW_SAMPLES, 2); // TODO: Hardcoded

	if ((context->window = glfwCreateWindow(context->options.window_size.x, context->options.window_size.y,
	                                        context->options.caption, NULL, NULL)) == NULL)
	{
		StatusSet(st, "ContextCreate", STATUS_ERROR, "Creating GLFW window");
		goto return_failure;
	}

	glfwMakeContextCurrent(context->window);

	glfwSetWindowSizeLimits(context->window, context->options.windows_min_size.x, context->options.windows_min_size.y,
	                        GLFW_DONT_CARE, GLFW_DONT_CARE);

	glfwSetWindowUserPointer(context->window, context);
	glfwSetFramebufferSizeCallback(context->window, sResizeCallback);
	glfwSetKeyCallback(context->window, sKeyboardCallback);

	glfwSwapInterval(0); // TODO: Hardcoded

	if (context->options.fullscreen == true)
	{
		vid_mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwSetWindowMonitor(context->window, glfwGetPrimaryMonitor(), 0, 0, vid_mode->width, vid_mode->height,
		                     GLFW_DONT_CARE);
	}

	// Joystick initialization
	context->joystick_id = -1;

	for (int i = GLFW_JOYSTICK_1; i < GLFW_JOYSTICK_LAST; i++)
	{
		if (glfwJoystickPresent(i) == GLFW_TRUE)
		{
			printf("Gamepad found: '%s'\n", glfwGetJoystickName(i));

			if (context->joystick_id == -1)
				context->joystick_id = i;
		}
	}

	// Context specific initialization
	timespec_get(&context->last_update, TIME_UTC);
	timespec_get(&context->one_second_counter, TIME_UTC);

	context->events.window.width = context->options.window_size.x;
	context->events.window.height = context->options.window_size.y;

	// GL specific initialization
	ViewportResize(context->options.scale_mode, context->options.window_size.x, context->options.window_size.y,
	               context->options.steps_size.x, context->options.steps_size.y);

	glClearColor(context->options.clean_color.x, context->options.clean_color.y, context->options.clean_color.z, 1.0);
	glEnableVertexAttribArray(ATTRIBUTE_POSITION);
	glEnableVertexAttribArray(ATTRIBUTE_UV);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

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
}


/*-----------------------------

 ContextUpdate()
-----------------------------*/
void ContextUpdate(struct Context* context, struct Events* out_events)
{
	glfwSwapBuffers(context->window);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
	const float* joy_axes = NULL;
	int joy_buttons_no = 0;
	int joy_axes_no = 0;

	if (out_events != NULL)
	{
		glfwPollEvents(); // TODO: This global state function did not goes well with MULTIPLE_CONTEXTS_TEST
		                  // Joystick functions did not require PollEvents()

		if (context->joystick_id != -1)
		{
			if ((joy_buttons = glfwGetJoystickButtons(context->joystick_id, &joy_buttons_no)) != NULL &&
			    (joy_axes = glfwGetJoystickAxes(context->joystick_id, &joy_axes_no)) != NULL)
			{
				out_events->a = (joy_buttons_no >= 1 && joy_buttons[0] == GLFW_TRUE) ? true : false;
				out_events->b = (joy_buttons_no >= 2 && joy_buttons[1] == GLFW_TRUE) ? true : false;
				out_events->x = (joy_buttons_no >= 3 && joy_buttons[2] == GLFW_TRUE) ? true : false;
				out_events->y = (joy_buttons_no >= 4 && joy_buttons[3] == GLFW_TRUE) ? true : false;

				out_events->lb = (joy_buttons_no >= 5 && joy_buttons[4] == GLFW_TRUE) ? true : false;
				out_events->rb = (joy_buttons_no >= 6 && joy_buttons[5] == GLFW_TRUE) ? true : false;

				out_events->view = (joy_buttons_no >= 7 && joy_buttons[6] == GLFW_TRUE) ? true : false;
				out_events->menu = (joy_buttons_no >= 8 && joy_buttons[7] == GLFW_TRUE) ? true : false;
				out_events->guide = (joy_buttons_no >= 9 && joy_buttons[8] == GLFW_TRUE) ? true : false;

				out_events->ls = (joy_buttons_no >= 5 && joy_buttons[9] == GLFW_TRUE) ? true : false;
				out_events->rs = (joy_buttons_no >= 6 && joy_buttons[10] == GLFW_TRUE) ? true : false;

				out_events->left_analog.h = (joy_axes_no >= 1) ? joy_axes[0] : 0.0;
				out_events->left_analog.v = (joy_axes_no >= 2) ? joy_axes[1] : 0.0;
				out_events->left_analog.t = (joy_axes_no >= 3) ? joy_axes[2] : 0.0;

				out_events->right_analog.h = (joy_axes_no >= 4) ? joy_axes[3] : 0.0;
				out_events->right_analog.v = (joy_axes_no >= 5) ? joy_axes[4] : 0.0;
				out_events->right_analog.t = (joy_axes_no >= 6) ? joy_axes[5] : 0.0;

				out_events->pad.h = (joy_axes_no >= 7) ? joy_axes[6] : 0.0;
				out_events->pad.v = (joy_axes_no >= 8) ? joy_axes[7] : 0.0;
			}
			else
			{
				if (glfwJoystickPresent(context->joystick_id) == GLFW_FALSE)
				{
					context->joystick_id = -1;
					printf("Gamepad disconnected\n");
				}
			}
		}

		// Misc
		out_events->window.resize = context->events.window.resize;
		context->events.window.resize = false;

		out_events->window.width = context->events.window.width;
		out_events->window.height = context->events.window.height;
		out_events->window.close = (glfwWindowShouldClose(context->window) == GLFW_TRUE) ? true : false;

		out_events->time.frame_no = context->frame_no;
		out_events->time.one_second = one_second;
		out_events->time.betwen_frames = betwen_frames;

		context->last_update = current_time;
	}
}


/*-----------------------------

 ContextSetProgram()
-----------------------------*/
void ContextSetProgram(struct Context* context, const struct Program* program)
{
	if (program != context->current_program)
	{
		struct Matrix4 as_matrix = Matrix4LookAt(context->camera_components[1], context->camera_components[0],
		                                         (struct Vector3){0.0, 0.0, 1.0});

		context->current_program = program;
		context->u_projection = glGetUniformLocation(program->ptr, "projection");
		context->u_camera_projection = glGetUniformLocation(context->current_program->ptr, "camera_projection");
		context->u_camera_components = glGetUniformLocation(context->current_program->ptr, "camera_components");
		context->u_color_texture = glGetUniformLocation(context->current_program->ptr, "color_texture");

		glUseProgram(program->ptr);
		glUniformMatrix4fv(context->u_projection, 1, GL_FALSE, context->projection.e);
		glUniformMatrix4fv(context->u_camera_projection, 1, GL_FALSE, as_matrix.e);
		glUniform3fv(context->u_camera_components, 2, (float*)&context->camera_components);
		glUniform1i(context->u_color_texture, 0); // Texture unit 0
	}
}


/*-----------------------------

 ContextSetProjection()
-----------------------------*/
void ContextSetProjection(struct Context* context, struct Matrix4 matrix)
{
	memcpy(&context->projection, &matrix, sizeof(struct Matrix4));

	if (context->current_program != NULL)
		glUniformMatrix4fv(context->u_projection, 1, GL_FALSE, context->projection.e);
}


/*-----------------------------

 ContextSetCamera()
-----------------------------*/
void ContextSetCamera(struct Context* context, struct Vector3 target, struct Vector3 origin)
{
	context->camera_components[0] = target;
	context->camera_components[1] = origin;

	if (context->current_program != NULL)
	{
		struct Matrix4 as_matrix = Matrix4LookAt(origin, target, (struct Vector3){0.0, 0.0, 1.0});

		glUniformMatrix4fv(context->u_camera_projection, 1, GL_FALSE, as_matrix.e);
		glUniform3fv(context->u_camera_components, 2, (float*)&context->camera_components);
	}
}


/*-----------------------------

 ContextDraw()
-----------------------------*/
void ContextDraw(struct Context* context, const struct Vertices* vertices, const struct Index* index,
                 const struct Texture* color)
{
	glBindBuffer(GL_ARRAY_BUFFER, vertices->ptr);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index->ptr);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, color->ptr);

	glVertexAttribPointer(ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), NULL);
	glVertexAttribPointer(ATTRIBUTE_UV, 2, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), ((float*)NULL) + 3);

	// glDrawElements(GL_LINES, index->length, GL_UNSIGNED_SHORT, NULL);
	glDrawElements(GL_TRIANGLES, index->length, GL_UNSIGNED_SHORT, NULL);
}
