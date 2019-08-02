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

 [game-camera.c]
 - Alexander Brandt 2019
-----------------------------*/

#include "game.h"


/*-----------------------------

 GameCameraStart()
-----------------------------*/
void GameCameraStart(struct Entity* self)
{
	struct GameCamera* data = self->data;
	(void)data;

	self->position = (struct Vector3){5000.0, 5000.0, 5000.0};
}


/*-----------------------------

 GameCameraDelete()
-----------------------------*/
void GameCameraDelete(struct Entity* self)
{
	struct GameCamera* data = self->data;
	(void)data;
}


/*-----------------------------

 GameCameraThink()
-----------------------------*/
void GameCameraThink(struct Entity* self, float delta)
{
	struct GameCamera* data = self->data;
	(void)data;

	struct Vector3 temp_v;

	// Forward, backward
	if (fabs(g_input_specs.left_analog.v) > 0.1) // Dead zone
	{
		float x = sin(DEG_TO_RAD(self->angle.z)) * sin(DEG_TO_RAD(self->angle.x));
		float y = cos(DEG_TO_RAD(self->angle.z)) * sin(DEG_TO_RAD(self->angle.x));
		float z = cos(DEG_TO_RAD(self->angle.x));

		temp_v = Vector3Set(x, y, z);
		temp_v = Vector3Scale(temp_v, g_input_specs.left_analog.v * 50.0 * delta);

		self->position = Vector3Add(self->position, temp_v);
	}

	// Stride left, right
	if (fabs(g_input_specs.left_analog.h) > 0.1) // Dead zone
	{
		float x = sin(DEG_TO_RAD(self->angle.z + 90.0));
		float y = cos(DEG_TO_RAD(self->angle.z + 90.0));
		float z = 0.0;

		temp_v = Vector3Set(x, y, z);
		temp_v = Vector3Scale(temp_v, g_input_specs.left_analog.h * 50.0 * delta);

		self->position = Vector3Add(self->position, temp_v);
	}

	// Look up, down
	if (fabs(g_input_specs.right_analog.v) > 0.2)
	{
		self->angle.x += g_input_specs.right_analog.v * 4.0 * delta;
	}

	// Look left, right
	if (fabs(g_input_specs.right_analog.h) > 0.2)
	{
		self->angle.z += g_input_specs.right_analog.h * 4.0 * delta;
	}
}
