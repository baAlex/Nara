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

 [context-input.c]
 - Alexander Brandt 2019
-----------------------------*/

#include "context-private.h"


/*-----------------------------

 FindGamedpad()
-----------------------------*/
inline int FindGamedpad()
{
	int winner = -1; // The first one wins

	for (int i = GLFW_JOYSTICK_1; i < GLFW_JOYSTICK_LAST; i++)
	{
		if (glfwJoystickPresent(i) == GLFW_TRUE)
		{
			printf("Gamepad found: '%s'\n", glfwGetJoystickName(i));

			if (winner == -1)
				winner = i;
		}
	}

	return winner;
}


/*-----------------------------

 InputStep()
-----------------------------*/
static inline float sCombineAxes(float a, float b)
{
	if (signbit((a + b) / 2.f) == 0) // 0 = Positive
		return fmaxf(a, b);

	return fminf(a, b);
}

void InputStep(struct Context* context)
{
	// NOTE: Gamepad functions did not require PollEvents()

	const uint8_t* buttons = NULL;
	const float* axes = NULL;
	int buttons_no = 0;
	int axes_no = 0;

	bool check_disconnection = false;

	memset(&context->gamepad, 0, sizeof(struct ContextEvents));
	memset(&context->combined, 0, sizeof(struct ContextEvents));

	// Gamepad input...
	if (context->active_gamepad != -1)
	{
		// While keyboard and mouse uses callbacks, this well... close your eyes
		if ((buttons = glfwGetJoystickButtons(context->active_gamepad, &buttons_no)) != NULL)
		{
			context->gamepad.a = (buttons_no >= 1 && buttons[0] == GLFW_TRUE) ? true : false;
			context->gamepad.b = (buttons_no >= 2 && buttons[1] == GLFW_TRUE) ? true : false;
			context->gamepad.x = (buttons_no >= 3 && buttons[2] == GLFW_TRUE) ? true : false;
			context->gamepad.y = (buttons_no >= 4 && buttons[3] == GLFW_TRUE) ? true : false;

			context->gamepad.lb = (buttons_no >= 5 && buttons[4] == GLFW_TRUE) ? true : false;
			context->gamepad.rb = (buttons_no >= 6 && buttons[5] == GLFW_TRUE) ? true : false;

			context->gamepad.view = (buttons_no >= 7 && buttons[6] == GLFW_TRUE) ? true : false;
			context->gamepad.menu = (buttons_no >= 8 && buttons[7] == GLFW_TRUE) ? true : false;
			context->gamepad.guide = (buttons_no >= 9 && buttons[8] == GLFW_TRUE) ? true : false;

			context->gamepad.ls = (buttons_no >= 5 && buttons[9] == GLFW_TRUE) ? true : false;
			context->gamepad.rs = (buttons_no >= 6 && buttons[10] == GLFW_TRUE) ? true : false;
		}
		else
		{
			context->gamepad.a = context->gamepad.b = context->gamepad.x = context->gamepad.y = false;
			context->gamepad.lb = context->gamepad.rb = false;
			context->gamepad.view = context->gamepad.menu = context->gamepad.guide = false;
			context->gamepad.ls = context->gamepad.rs = false;
			check_disconnection = true;
		}

		if ((axes = glfwGetJoystickAxes(context->active_gamepad, &axes_no)) != NULL)
		{
			context->gamepad.left_analog.h = (axes_no >= 1) ? axes[0] : 0.0;
			context->gamepad.left_analog.v = (axes_no >= 2) ? axes[1] : 0.0;
			context->gamepad.left_analog.t = (axes_no >= 3) ? axes[2] : 0.0;

			context->gamepad.right_analog.h = (axes_no >= 4) ? axes[3] : 0.0;
			context->gamepad.right_analog.v = (axes_no >= 5) ? axes[4] : 0.0;
			context->gamepad.right_analog.t = (axes_no >= 6) ? axes[5] : 0.0;

			context->gamepad.pad.h = (axes_no >= 7) ? axes[6] : 0.0;
			context->gamepad.pad.v = (axes_no >= 8) ? axes[7] : 0.0;
		}
		else
		{
			context->gamepad.left_analog.h = context->gamepad.left_analog.v = 0.0;
			context->gamepad.right_analog.h = context->gamepad.right_analog.v = 0.0;
			context->gamepad.left_analog.t = context->gamepad.right_analog.t = -1.0;
			context->gamepad.pad.h = context->gamepad.pad.v = 0.0;
			check_disconnection = true;
		}

		// Is disconnected?, lets find a new one
		if (check_disconnection == true)
		{
			if (glfwJoystickPresent(context->active_gamepad) == GLFW_FALSE)
			{
				printf("Gamepad disconnected!\n");
				context->active_gamepad = FindGamedpad();
			}
		}
	}

	// ... But there are no active gamepad!
	else
	{
		memset(&context->gamepad, 0, sizeof(struct ContextEvents));
	}

	// Combine both gamepad and keyboard input into final events
	context->combined.a = (context->keyboard.a == true || context->gamepad.a == GLFW_TRUE) ? true : false;
	context->combined.b = (context->keyboard.b == true || context->gamepad.b == GLFW_TRUE) ? true : false;
	context->combined.x = (context->keyboard.x == true || context->gamepad.x == GLFW_TRUE) ? true : false;
	context->combined.y = (context->keyboard.y == true || context->gamepad.y == GLFW_TRUE) ? true : false;

	context->combined.lb = (context->keyboard.lb == true || context->gamepad.lb == GLFW_TRUE) ? true : false;
	context->combined.rb = (context->keyboard.rb == true || context->gamepad.rb == GLFW_TRUE) ? true : false;

	context->combined.view = (context->keyboard.view == true || context->gamepad.view == GLFW_TRUE) ? true : false;
	context->combined.menu = (context->keyboard.menu == true || context->gamepad.menu == GLFW_TRUE) ? true : false;
	context->combined.guide = (context->keyboard.guide == true || context->gamepad.guide == GLFW_TRUE) ? true : false;

	context->combined.ls = (context->keyboard.ls == true || context->gamepad.ls == GLFW_TRUE) ? true : false;
	context->combined.rs = (context->keyboard.rs == true || context->gamepad.rs == GLFW_TRUE) ? true : false;

	context->combined.left_analog.h = sCombineAxes(context->gamepad.left_analog.h, context->keyboard.left_analog.h);
	context->combined.left_analog.v = sCombineAxes(context->gamepad.left_analog.v, context->keyboard.left_analog.v);
	context->combined.left_analog.t = sCombineAxes(context->gamepad.left_analog.t, context->keyboard.left_analog.t);

	context->combined.right_analog.h = sCombineAxes(context->gamepad.right_analog.h, context->keyboard.right_analog.h);
	context->combined.right_analog.v = sCombineAxes(context->gamepad.right_analog.v, context->keyboard.right_analog.v);
	context->combined.right_analog.t = sCombineAxes(context->gamepad.right_analog.t, context->keyboard.right_analog.t);

	context->combined.pad.h = sCombineAxes(context->gamepad.pad.h, context->keyboard.pad.h);
	context->combined.pad.v = sCombineAxes(context->gamepad.pad.v, context->keyboard.pad.v);
}


/*-----------------------------

 KeyboardCallback()
-----------------------------*/
static inline float sKeysToAxe(int action, int current_key, float current_axe, int key_positive, int key_negative)
{
	if (action == GLFW_PRESS)
	{
		if (current_key == key_positive)
			current_axe += 1.0;
		else if (current_key == key_negative)
			current_axe -= 1.0;
	}
	else
	{
		if (current_key == key_positive)
			current_axe -= 1.0;
		else if (current_key == key_negative)
			current_axe += 1.0;
	}

	return current_axe;
}

void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	(void)scancode;
	(void)mods;

	struct Context* context = glfwGetWindowUserPointer(window);

	if (action == GLFW_REPEAT)
		return;

	// Two states old school buttons
	switch (key)
	{
	case GLFW_KEY_Q: context->keyboard.lb = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_E: context->keyboard.rb = (action == GLFW_PRESS) ? true : false; break;

	case GLFW_KEY_SPACE: context->keyboard.view = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_ENTER: context->keyboard.menu = (action == GLFW_PRESS) ? true : false; break;

	case GLFW_KEY_F1: context->keyboard.guide = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_T: context->keyboard.ls = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_Y: context->keyboard.rs = (action == GLFW_PRESS) ? true : false; break;

	default: break;
	}

	// Hey kid, do you wanna emulate analog sticks?
	// TODO: Left trigger
	// TODO: right trigger

	context->keyboard.left_analog.v = sKeysToAxe(action, key, context->keyboard.left_analog.v, GLFW_KEY_S, GLFW_KEY_W);
	context->keyboard.left_analog.h = sKeysToAxe(action, key, context->keyboard.left_analog.h, GLFW_KEY_D, GLFW_KEY_A);

	context->keyboard.right_analog.v = sKeysToAxe(action, key, context->keyboard.right_analog.v, GLFW_KEY_DOWN, GLFW_KEY_UP);
	context->keyboard.right_analog.h = sKeysToAxe(action, key, context->keyboard.right_analog.h, GLFW_KEY_RIGHT, GLFW_KEY_LEFT);

	context->keyboard.pad.v = sKeysToAxe(action, key, context->keyboard.pad.v, GLFW_KEY_K, GLFW_KEY_I);
	context->keyboard.pad.h = sKeysToAxe(action, key, context->keyboard.pad.h, GLFW_KEY_J, GLFW_KEY_L);
}
