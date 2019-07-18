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
void ContextInputStep(struct ContextInput* state)
{
	// NOTE: Joystick functions did not require PollEvents()

	const uint8_t* buttons = NULL;
	const float* axes = NULL;
	int buttons_no = 0;
	int axes_no = 0;

	bool check_disconnection = false;

	// Using active gamepad...
	if (state->active_gamepad != -1)
	{
		if ((buttons = glfwGetJoystickButtons(state->active_gamepad, &buttons_no)) != NULL)
		{
			state->specs.a = (buttons_no >= 1 && buttons[0] == GLFW_TRUE) ? true : false;
			state->specs.b = (buttons_no >= 2 && buttons[1] == GLFW_TRUE) ? true : false;
			state->specs.x = (buttons_no >= 3 && buttons[2] == GLFW_TRUE) ? true : false;
			state->specs.y = (buttons_no >= 4 && buttons[3] == GLFW_TRUE) ? true : false;

			state->specs.lb = (buttons_no >= 5 && buttons[4] == GLFW_TRUE) ? true : false;
			state->specs.rb = (buttons_no >= 6 && buttons[5] == GLFW_TRUE) ? true : false;

			state->specs.view = (buttons_no >= 7 && buttons[6] == GLFW_TRUE) ? true : false;
			state->specs.menu = (buttons_no >= 8 && buttons[7] == GLFW_TRUE) ? true : false;
			state->specs.guide = (buttons_no >= 9 && buttons[8] == GLFW_TRUE) ? true : false;

			state->specs.ls = (buttons_no >= 5 && buttons[9] == GLFW_TRUE) ? true : false;
			state->specs.rs = (buttons_no >= 6 && buttons[10] == GLFW_TRUE) ? true : false;
		}
		else
		{
			state->specs.a = state->specs.b = state->specs.x = state->specs.y = false;
			state->specs.lb = state->specs.rb = false;
			state->specs.view = state->specs.menu = state->specs.guide = false;
			state->specs.ls = state->specs.rs = false;
			check_disconnection = true;
		}

		if ((axes = glfwGetJoystickAxes(state->active_gamepad, &axes_no)) != NULL)
		{
			state->specs.left_analog.h = (axes_no >= 1) ? axes[0] : 0.0;
			state->specs.left_analog.v = (axes_no >= 2) ? axes[1] : 0.0;
			state->specs.left_analog.t = (axes_no >= 3) ? axes[2] : 0.0;

			state->specs.right_analog.h = (axes_no >= 4) ? axes[3] : 0.0;
			state->specs.right_analog.v = (axes_no >= 5) ? axes[4] : 0.0;
			state->specs.right_analog.t = (axes_no >= 6) ? axes[5] : 0.0;

			state->specs.pad.h = (axes_no >= 7) ? axes[6] : 0.0;
			state->specs.pad.v = (axes_no >= 8) ? axes[7] : 0.0;
		}
		else
		{
			state->specs.left_analog.h = state->specs.left_analog.v = 0.0;
			state->specs.right_analog.h = state->specs.right_analog.v = 0.0;
			state->specs.left_analog.t = state->specs.right_analog.t = -1.0;
			state->specs.pad.h = state->specs.pad.v = 0.0;
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
		memset(&state->specs, 0, sizeof(struct InputSpecifications));
	}
}
