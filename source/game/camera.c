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


#define ANALOG_DEAD_ZONE 0.2
#define MOVEMENT_SPEED 2.5
#define LOOK_SPEED 3.0


/*-----------------------------

 GameCameraStart()
-----------------------------*/
void* GameCameraStart()
{
	struct GameCamera* self = malloc(sizeof(struct GameCamera)); // TODO, check error

	self->co.position = (struct Vector3){128.0, 128.0, 256.0};
	self->co.angle = (struct Vector3){-67.5, 0.0, 45.0};

	return self;
}


/*-----------------------------

 GameCameraDelete()
-----------------------------*/
void GameCameraDelete(void* raw_self)
{
	struct GameCamera* self = raw_self;
	free(self);
}


/*-----------------------------

 GameCameraThink()
-----------------------------*/
struct EntityCommon GameCameraThink(void* raw_self, const struct EntityInput* input)
{
	struct GameCamera* self = raw_self;
	struct Vector3 temp_v;

	// Forward, backward
	if (fabs(input->left_analog.v) > ANALOG_DEAD_ZONE)
	{
		float x = sinf(DEG_TO_RAD(self->co.angle.z)) * sinf(DEG_TO_RAD(self->co.angle.x));
		float y = cosf(DEG_TO_RAD(self->co.angle.z)) * sinf(DEG_TO_RAD(self->co.angle.x));
		float z = cosf(DEG_TO_RAD(self->co.angle.x));

		temp_v = Vector3Set(x, y, z);
		temp_v = Vector3Scale(temp_v, input->left_analog.v * MOVEMENT_SPEED * input->delta);

		self->co.position = Vector3Add(self->co.position, temp_v);
	}

	// Stride left, right
	if (fabs(input->left_analog.h) > ANALOG_DEAD_ZONE)
	{
		float x = sinf(DEG_TO_RAD(self->co.angle.z + 90.0));
		float y = cosf(DEG_TO_RAD(self->co.angle.z + 90.0));
		float z = 0.0;

		temp_v = Vector3Set(x, y, z);
		temp_v = Vector3Scale(temp_v, input->left_analog.h * MOVEMENT_SPEED * input->delta);

		self->co.position = Vector3Add(self->co.position, temp_v);
	}

	// Look up, down
	if (fabs(input->right_analog.v) > ANALOG_DEAD_ZONE)
	{
		self->co.angle.x += input->right_analog.v * LOOK_SPEED * input->delta;

		if(self->co.angle.x < -180.0)
			self->co.angle.x = -180.0;
		if(self->co.angle.x > 0.0)
			self->co.angle.x = 0.0;
	}

	// Look left, right
	if (fabs(input->right_analog.h) > ANALOG_DEAD_ZONE)
	{
		self->co.angle.z += input->right_analog.h * LOOK_SPEED * input->delta;
	}

	return self->co;
}
