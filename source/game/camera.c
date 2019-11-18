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

 [camera.c]
 - Alexander Brandt 2019
-----------------------------*/

#include "game.h"


#define ANALOG_DEAD_ZONE 0.2f
#define MOVEMENT_SPEED 2.5f
#define LOOK_SPEED 3.0f


struct Camera
{
	struct Vector3 position;
	struct Vector3 angle;

	float acceleration; // Placeholder
	float inertia;      // "
};


/*-----------------------------

 GameCameraStart()
-----------------------------*/
void* GameCameraStart(const struct EntityCommon* initial_state)
{
	struct Camera* camera = malloc(sizeof(struct Camera));

	if (camera != NULL)
	{
		camera->position = initial_state->position;
		camera->angle = initial_state->angle;
	}

	return camera;
}


/*-----------------------------

 GameCameraDelete()
-----------------------------*/
void GameCameraDelete(void* blob)
{
	struct Camera* camera = blob;
	free(camera);
}


/*-----------------------------

 GameCameraThink()
-----------------------------*/
int GameCameraThink(void* blob, const struct EntityInput* input, struct EntityCommon* out_state)
{
	struct Camera* camera = blob;
	struct Vector3 temp;

	struct Vector3 forward;
	struct Vector3 left;
	struct Vector3 up;

	VectorAxes(camera->angle, &forward, &left, &up);

	// Forward, backward
	if (fabs(input->left_analog.v) > ANALOG_DEAD_ZONE)
	{
		temp = Vector3Scale(forward, (-1.0f) * input->left_analog.v * MOVEMENT_SPEED * input->delta);
		camera->position = Vector3Add(camera->position, temp);
	}

	// Stride left, right
	if (fabs(input->left_analog.h) > ANALOG_DEAD_ZONE)
	{
		temp = Vector3Scale(left, input->left_analog.h * MOVEMENT_SPEED * input->delta);
		camera->position = Vector3Add(camera->position, temp);
	}

	// Up
	if (fabs(input->right_analog.t) > ANALOG_DEAD_ZONE)
	{
		temp = Vector3Scale(up, input->right_analog.t * MOVEMENT_SPEED * input->delta);
		camera->position = Vector3Add(camera->position, temp);
	}

	// Down
	if (fabs(input->left_analog.t) > ANALOG_DEAD_ZONE)
	{
		temp = Vector3Scale(up, (-1.0f) * input->left_analog.t * MOVEMENT_SPEED * input->delta);
		camera->position = Vector3Add(camera->position, temp);
	}

	// Look up, down
	if (fabs(input->right_analog.v) > ANALOG_DEAD_ZONE)
	{
		camera->angle.x += input->right_analog.v * LOOK_SPEED * input->delta;

		if (camera->angle.x < -180.0f)
			camera->angle.x = -180.0f;
		if (camera->angle.x > 0.0f)
			camera->angle.x = 0.0f;
	}

	// Look left, right
	if (fabs(input->right_analog.h) > ANALOG_DEAD_ZONE)
	{
		camera->angle.z += input->right_analog.h * LOOK_SPEED * input->delta;
	}

	// Bye
	out_state->position = camera->position;
	out_state->angle = camera->angle;

	return 0;
}
