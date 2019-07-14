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

 [nara.c]
 - Alexander Brandt 2019
-----------------------------*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "context.h"
#include "terrain.h"

extern const uint8_t g_terrain_vertex[];
extern const uint8_t g_terrain_fragment[];

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

#define DEG_TO_RAD(d) ((d)*M_PI / 180.0)
#define RAD_TO_DEG(r) ((r)*180.0 / M_PI)

#define FOV 45

#define WINDOWS_MIN_WIDTH 220
#define WINDOWS_MIN_HEIGHT 100


/*-----------------------------

 sCameraMove()
-----------------------------*/
static void sCameraMove(struct Context* context, struct Events* evn, struct Vector3* target, float* distance)
{
	// The following lazy code should work
	// until support of analog input

	bool update = false;
	float speed = 0.5 * (evn->time.betwen_frames / 33.3333); // Delta calculation

	struct Vector3 origin = {0};
	struct Vector3 v = Vector3Subtract(Vector3Add(*target, (struct Vector3){1.0, 1.0, 1.0}), *target);

	v.z = 0.0;
	v = Vector3Normalize(v);

	// Up, down
	if (evn->lb == true && evn->rb == false)
	{
		*distance += speed;

		origin = Vector3Add(*target, (struct Vector3){*distance, *distance, *distance});
		update = true;
	}
	else if (evn->rb == true && evn->lb == false)
	{
		if (*distance > 1.0)
			*distance -= speed;

		origin = Vector3Add(*target, (struct Vector3){*distance, *distance, *distance});
		update = true;
	}
	else
	{
		origin = Vector3Add(*target, (struct Vector3){*distance, *distance, *distance});
	}

	// Forward, backward
	if (evn->a == true && evn->y == false)
	{
		v = Vector3Scale(v, speed);

		*target = Vector3Add(*target, v);
		origin = Vector3Add(origin, v);
		update = true;
	}
	else if (evn->y == true && evn->a == false)
	{
		v = Vector3Scale(v, speed);

		*target = Vector3Subtract(*target, v);
		origin = Vector3Subtract(origin, v);
		update = true;
	}

	// Left, right
	if (evn->x == true && evn->b == false)
	{
		float angle = atan2f(v.y, v.x);

		v.x = cosf(angle + DEG_TO_RAD(90.0));
		v.y = sinf(angle + DEG_TO_RAD(90.0));
		v = Vector3Scale(v, speed);

		*target = Vector3Subtract(*target, v);
		origin = Vector3Subtract(origin, v);
		update = true;
	}
	else if (evn->b == true && evn->x == false)
	{
		float angle = atan2f(v.y, v.x);

		v.x = cosf(angle + DEG_TO_RAD(90.0));
		v.y = sinf(angle + DEG_TO_RAD(90.0));
		v = Vector3Scale(v, speed);

		*target = Vector3Add(*target, v);
		origin = Vector3Add(origin, v);
		update = true;
	}

	if (update == true)
		ContextSetCamera(context, *target, origin);
}


/*-----------------------------

 main()
-----------------------------*/
int main()
{
	struct Context* context = NULL;
	struct Events evn = {0};
	struct Status st = {0};

	struct Program* terrain_program = NULL;
	struct Terrain* terrain = NULL;

	struct Matrix4 projection;
	struct Vector3 camera_target = {50.0, 50.0, 0.0};
	float camera_distance = 50.0;

	printf("Nara v0.1\n");
	printf("- Lib-Japan v%i.%i.%i\n", JAPAN_VERSION_MAJOR, JAPAN_VERSION_MINOR, JAPAN_VERSION_PATCH);
	printf("\n");

	// Initialization
	context = ContextCreate((struct ContextOptions)
	{
		.caption = "Nara",
		.window_size = {WINDOWS_MIN_WIDTH * 4, WINDOWS_MIN_HEIGHT * 4},
		.windows_min_size = {WINDOWS_MIN_WIDTH, WINDOWS_MIN_HEIGHT},
		.scale_mode = SCALE_MODE_STRETCH,
		.fullscreen = false,
		.clean_color = {0.80, 0.82, 0.84}
	}, &st);

	if (context == NULL)
		goto return_failure;

	// Resources
	struct TerrainOptions terrain_options = {0};

	terrain_options.heightmap_filename = "./resources/heightmap.sgi";
	terrain_options.colormap_filename = "./resources/colormap.sgi";
	terrain_options.width = 100;
	terrain_options.height = 100;
	terrain_options.elevation = 20;

	if ((terrain_program = ProgramCreate((char*)g_terrain_vertex, (char*)g_terrain_fragment, &st)) == NULL ||
		(terrain = TerrainCreate(terrain_options, &st)) == NULL)
		goto return_failure;

	projection = Matrix4Perspective(FOV, (float)WINDOWS_MIN_WIDTH / (float)WINDOWS_MIN_HEIGHT, 0.1, 4000.0);

	ContextSetProjection(context, projection);
	ContextSetProgram(context, terrain_program);
	ContextSetCamera(context, camera_target,
					 Vector3Add(camera_target, (struct Vector3){camera_distance, camera_distance, camera_distance}));

	// Yay!
	while (1)
	{
		ContextUpdate(context, &evn);
		ContextDraw(context, terrain->vertices, terrain->index, terrain->color);

		// Events
		/*printf("%s %s %s %s %s %s %s %s %s %s %s\n", evn.a ? "a" : "-", evn.b ? "b" : "-", evn.x ? "x" : "-",
			   evn.y ? "y" : "-", evn.lb ? "lb" : "--", evn.rb ? "rb" : "--", evn.view ? "view" : "----",
			   evn.menu ? "menu" : "----", evn.guide ? "guide" : "-----", evn.ls ? "ls" : "--", evn.rs ? "rs" : "--");*/

		sCameraMove(context, &evn, &camera_target, &camera_distance);

		if (evn.window.resize == true)
		{
			printf("Window resized: %ix%i px\n", evn.window.width, evn.window.height);

			projection = Matrix4Perspective(FOV, (float)evn.window.width / (float)evn.window.height, 0.1, 4000.0);
			ContextSetProjection(context, projection);
		}

		if (evn.window.close == true)
			break;
	}

	// Bye!
	ProgramDelete(terrain_program);
	TerrainDelete(terrain);
	ContextDelete(context);

	return EXIT_SUCCESS;

return_failure:

	StatusPrint(st);
	if (terrain != NULL)
		TerrainDelete(terrain);
	if (terrain_program != NULL)
		ProgramDelete(terrain_program);
	if (context != NULL)
		ContextDelete(context);

	return EXIT_FAILURE;
}
