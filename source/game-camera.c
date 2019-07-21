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
	printf("### Hello, Camera %p here!\n", (void*)self);

	data->target = (struct Vector3){5000.0, 5000.0, 0.0};
	data->distance = 5000.0;

	ContextSetCamera(g_context, data->target,
	                 Vector3Add(data->target, (struct Vector3){0.0, data->distance, data->distance}));
}


/*-----------------------------

 GameCameraDelete()
-----------------------------*/
void GameCameraDelete(struct Entity* self)
{
	struct GameCamera* data = self->data;
	(void)data;

	printf("### Bye, was a pleasure for Camera %p\n", (void*)self);
}


/*-----------------------------

 GameCameraThink()
-----------------------------*/
void GameCameraThink(struct Entity* self, float delta)
{
	struct GameCamera* data = self->data;

	bool update = false;
	struct Vector3 temp_origin = {0};
	struct Vector3 v = {0};

	float speed = 50.0 * delta;

	// Up, down
	if (g_input_specs.lb == true && g_input_specs.rb == false)
	{
		data->distance += speed / 2.0;
		update = true;
	}
	else if (g_input_specs.rb == true && g_input_specs.lb == false)
	{
		data->distance -= speed / 2.0;
		update = true;
	}

	temp_origin = Vector3Add(data->target, (struct Vector3){0.0, data->distance, data->distance});

	// Current angle
	v = Vector3Subtract(temp_origin, data->target);
	v.z = 0.0;
	v = Vector3Normalize(v);

	// Forward, backward
	if (fabs(g_input_specs.left_analog.v) > 0.1) // Dead zone
	{
		v = Vector3Scale(v, powf((g_input_specs.left_analog.v), 2.0) * speed);

		if (g_input_specs.left_analog.v < 0.0)
			v = Vector3Invert(v);

		data->target = Vector3Add(data->target, v);
		temp_origin = Vector3Add(temp_origin, v);
		update = true;

		// Current angle (again for left/right calculation)
		v = Vector3Subtract(temp_origin, data->target);
		v.z = 0.0;
		v = Vector3Normalize(v);
	}

	// Left, right
	if (fabs(g_input_specs.left_analog.h) > 0.1)
	{
		float angle = atan2f(v.y, v.x);

		v.x = cosf(angle + DEG_TO_RAD(90.0));
		v.y = sinf(angle + DEG_TO_RAD(90.0));
		v = Vector3Scale(v, powf(g_input_specs.left_analog.h, 2.0) * (-speed));

		if (g_input_specs.left_analog.h < 0.0)
			v = Vector3Invert(v);

		data->target = Vector3Subtract(data->target, v);
		temp_origin = Vector3Subtract(temp_origin, v);
		update = true;
	}

	if (update == true)
		ContextSetCamera(g_context, data->target, temp_origin);
}
