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

 sFindGamedpad()
-----------------------------*/
static inline int sFindGamedpad()
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

 ContextInputInitialization()
-----------------------------*/
void ContextInputInitialization(struct ContextInput* state)
{
	memset(state, 0, sizeof(struct ContextInput));
	state->active_gamepad = sFindGamedpad();
}


/*-----------------------------

 ContextInputStep()
-----------------------------*/
static inline float sCombineAxes(float a, float b)
{
	if(signbit((a + b) / 2.f) == 0) // 0 = Positive
		return fmaxf(a, b);

	return fminf(a, b);
}

void ContextInputStep(struct ContextInput* state)
{
	// NOTE: Joystick functions did not require PollEvents()

	const uint8_t* buttons = NULL;
	const float* axes = NULL;
	int buttons_no = 0;
	int axes_no = 0;

	bool check_disconnection = false;
	struct InputSpecifications joy_specs = {0};

	// Gamepad input...
	if (state->active_gamepad != -1)
	{
		// While keyboard and mouse uses callbacks, this well... close your eyes
		if ((buttons = glfwGetJoystickButtons(state->active_gamepad, &buttons_no)) != NULL)
		{
			joy_specs.a = (buttons_no >= 1 && buttons[0] == GLFW_TRUE) ? true : false;
			joy_specs.b = (buttons_no >= 2 && buttons[1] == GLFW_TRUE) ? true : false;
			joy_specs.x = (buttons_no >= 3 && buttons[2] == GLFW_TRUE) ? true : false;
			joy_specs.y = (buttons_no >= 4 && buttons[3] == GLFW_TRUE) ? true : false;

			joy_specs.lb = (buttons_no >= 5 && buttons[4] == GLFW_TRUE) ? true : false;
			joy_specs.rb = (buttons_no >= 6 && buttons[5] == GLFW_TRUE) ? true : false;

			joy_specs.view = (buttons_no >= 7 && buttons[6] == GLFW_TRUE) ? true : false;
			joy_specs.menu = (buttons_no >= 8 && buttons[7] == GLFW_TRUE) ? true : false;
			joy_specs.guide = (buttons_no >= 9 && buttons[8] == GLFW_TRUE) ? true : false;

			joy_specs.ls = (buttons_no >= 5 && buttons[9] == GLFW_TRUE) ? true : false;
			joy_specs.rs = (buttons_no >= 6 && buttons[10] == GLFW_TRUE) ? true : false;
		}
		else
		{
			joy_specs.a = joy_specs.b = joy_specs.x = joy_specs.y = false;
			joy_specs.lb = joy_specs.rb = false;
			joy_specs.view = joy_specs.menu = joy_specs.guide = false;
			joy_specs.ls = joy_specs.rs = false;
			check_disconnection = true;
		}

		if ((axes = glfwGetJoystickAxes(state->active_gamepad, &axes_no)) != NULL)
		{
			joy_specs.left_analog.h = (axes_no >= 1) ? axes[0] : 0.0;
			joy_specs.left_analog.v = (axes_no >= 2) ? axes[1] : 0.0;
			joy_specs.left_analog.t = (axes_no >= 3) ? axes[2] : 0.0;

			joy_specs.right_analog.h = (axes_no >= 4) ? axes[3] : 0.0;
			joy_specs.right_analog.v = (axes_no >= 5) ? axes[4] : 0.0;
			joy_specs.right_analog.t = (axes_no >= 6) ? axes[5] : 0.0;

			joy_specs.pad.h = (axes_no >= 7) ? axes[6] : 0.0;
			joy_specs.pad.v = (axes_no >= 8) ? axes[7] : 0.0;
		}
		else
		{
			joy_specs.left_analog.h = joy_specs.left_analog.v = 0.0;
			joy_specs.right_analog.h = joy_specs.right_analog.v = 0.0;
			joy_specs.left_analog.t = joy_specs.right_analog.t = -1.0;
			joy_specs.pad.h = joy_specs.pad.v = 0.0;
			check_disconnection = true;
		}

		// Is disconnected?, lets find a new one
		if (check_disconnection == true)
		{
			if (glfwJoystickPresent(state->active_gamepad) == GLFW_FALSE)
			{
				printf("Gamepad disconnected!\n");
				state->active_gamepad = sFindGamedpad();
			}
		}
	}

	// ... But there are no active gamepad!
	else
	{
		memset(&joy_specs, 0, sizeof(struct InputSpecifications));
	}

	// Combine both gamepad and keyboard input into the final specification
	state->specs.a = (state->key_specs.a == true || joy_specs.a == GLFW_TRUE) ? true : false;
	state->specs.b = (state->key_specs.b == true || joy_specs.b == GLFW_TRUE) ? true : false;
	state->specs.x = (state->key_specs.x == true || joy_specs.x == GLFW_TRUE) ? true : false;
	state->specs.y = (state->key_specs.y == true || joy_specs.y == GLFW_TRUE) ? true : false;

	state->specs.lb = (state->key_specs.lb == true || joy_specs.lb == GLFW_TRUE) ? true : false;
	state->specs.rb = (state->key_specs.rb == true || joy_specs.rb == GLFW_TRUE) ? true : false;

	state->specs.view = (state->key_specs.view == true || joy_specs.view == GLFW_TRUE) ? true : false;
	state->specs.menu = (state->key_specs.menu == true || joy_specs.menu == GLFW_TRUE) ? true : false;
	state->specs.guide = (state->key_specs.guide == true || joy_specs.guide == GLFW_TRUE) ? true : false;

	state->specs.ls = (state->key_specs.ls == true || joy_specs.ls == GLFW_TRUE) ? true : false;
	state->specs.rs = (state->key_specs.rs == true || joy_specs.rs == GLFW_TRUE) ? true : false;

	state->specs.left_analog.h = sCombineAxes(joy_specs.left_analog.h, state->key_specs.left_analog.h);
	state->specs.left_analog.v = sCombineAxes(joy_specs.left_analog.v, state->key_specs.left_analog.v);
	state->specs.left_analog.t = sCombineAxes(joy_specs.left_analog.t, state->key_specs.left_analog.t);

	state->specs.right_analog.h = sCombineAxes(joy_specs.right_analog.h, state->key_specs.right_analog.h);
	state->specs.right_analog.v = sCombineAxes(joy_specs.right_analog.v, state->key_specs.right_analog.v);
	state->specs.right_analog.t = sCombineAxes(joy_specs.right_analog.t, state->key_specs.right_analog.t);

	state->specs.pad.h = sCombineAxes(joy_specs.pad.h, state->key_specs.pad.h);
	state->specs.pad.v = sCombineAxes(joy_specs.pad.v, state->key_specs.pad.v);
}


/*-----------------------------

 ReceiveKeyboardKey()
-----------------------------*/
static inline float sKeysToAxe(int action, int current_key, float current_axe, int key_positive, int key_negative)
{
	if(action == GLFW_PRESS)
	{
		if(current_key == key_positive)
			current_axe += 1.0;
		else if(current_key == key_negative)
			current_axe -= 1.0;
	}
	else
	{
		if(current_key == key_positive)
			current_axe -= 1.0;
		else if(current_key == key_negative)
			current_axe += 1.0;
	}

	return current_axe;
}

void ReceiveKeyboardKey(struct ContextInput* state, int key, int action)
{
	if(action == GLFW_REPEAT)
		return;

	// Two states old school buttons
	switch (key)
	{
	case GLFW_KEY_Q: state->key_specs.lb = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_E: state->key_specs.rb = (action == GLFW_PRESS) ? true : false; break;

	case GLFW_KEY_SPACE: state->key_specs.view = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_ENTER: state->key_specs.menu = (action == GLFW_PRESS) ? true : false; break;

	case GLFW_KEY_F1: state->key_specs.guide = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_T: state->key_specs.ls = (action == GLFW_PRESS) ? true : false; break;
	case GLFW_KEY_Y: state->key_specs.rs = (action == GLFW_PRESS) ? true : false; break;

	default: break;
	}

	// Hey kid, do you wanna emulate analog sticks?
	// TODO: Left trigger
	// TODO: right trigger

	state->key_specs.left_analog.v = sKeysToAxe(action, key, state->key_specs.left_analog.v, GLFW_KEY_S, GLFW_KEY_W);
	state->key_specs.left_analog.h = sKeysToAxe(action, key, state->key_specs.left_analog.h, GLFW_KEY_D, GLFW_KEY_A);

	state->key_specs.right_analog.v = sKeysToAxe(action, key, state->key_specs.right_analog.v, GLFW_KEY_DOWN, GLFW_KEY_UP);
	state->key_specs.right_analog.h = sKeysToAxe(action, key, state->key_specs.right_analog.h, GLFW_KEY_RIGHT, GLFW_KEY_LEFT);

	state->key_specs.pad.v = sKeysToAxe(action, key, state->key_specs.pad.v, GLFW_KEY_K, GLFW_KEY_I);
	state->key_specs.pad.h = sKeysToAxe(action, key, state->key_specs.pad.h, GLFW_KEY_J, GLFW_KEY_L);
}
