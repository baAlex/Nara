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
extern const uint8_t g_red_vertex[];
extern const uint8_t g_red_fragment[];

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

#define DEG_TO_RAD(d) ((d)*M_PI / 180.0)
#define RAD_TO_DEG(r) ((r)*180.0 / M_PI)

#define FOV 45

#define WINDOWS_MIN_WIDTH 220
#define WINDOWS_MIN_HEIGHT 100


struct WindowSpecifications window_specs;
struct TimeSpecifications time_specs;
struct InputSpecifications input_specs;


/*-----------------------------

 sCameraMove()
-----------------------------*/
static void sCameraMove(struct Context* context, struct Vector3* target, float* distance)
{
	bool update = false;
	struct Vector3 origin = {0};
	struct Vector3 v = {0};

	float speed = 100.0 * (time_specs.miliseconds_betwen / 33.3333); // Delta calculation

	// Up, down
	if (input_specs.lb == true && input_specs.rb == false)
	{
		*distance += speed / 4.0;
		update = true;
	}
	else if (input_specs.rb == true && input_specs.lb == false)
	{
		*distance -= speed / 4.0;
		update = true;
	}

	origin = Vector3Add(*target, (struct Vector3){*distance, *distance, *distance});

	// Current angle
	v = Vector3Subtract(Vector3Add(*target, (struct Vector3){1.0, 1.0, 1.0}), *target);
	v.z = 0.0;
	v = Vector3Normalize(v);

	// Forward, backward
	if (fabs(input_specs.left_analog.v) > 0.12) // Dead zone
	{
		v = Vector3Scale(v, input_specs.left_analog.v * speed); // TODO: linear!

		*target = Vector3Add(*target, v);
		origin = Vector3Add(origin, v);
		update = true;

		// Current angle (again for left/right calculation)
		v = Vector3Subtract(Vector3Add(*target, (struct Vector3){1.0, 1.0, 1.0}), *target);
		v.z = 0.0;
		v = Vector3Normalize(v);
	}

	// Left, right
	if (fabs(input_specs.left_analog.h) > 0.12)
	{
		float angle = atan2f(v.y, v.x);

		v.x = cosf(angle + DEG_TO_RAD(90.0));
		v.y = sinf(angle + DEG_TO_RAD(90.0));
		v = Vector3Scale(v, (-input_specs.left_analog.h) * speed);

		*target = Vector3Subtract(*target, v);
		origin = Vector3Subtract(origin, v);
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
	struct Status st = {0};

	struct Program* terrain_program = NULL;
	struct Terrain* terrain = NULL;

	struct Matrix4 projection;
	struct Vector3 camera_target = {5000.0, 5000.0, 0.0}; // 5 km
	float camera_distance = 5000.0;                       // 5 km

	printf("Nara v0.1\n");
	printf("- Lib-Japan v%i.%i.%i\n", JAPAN_VERSION_MAJOR, JAPAN_VERSION_MINOR, JAPAN_VERSION_PATCH);
	printf("\n");

	// Initialization
	context = ContextCreate((struct ContextOptions){
		.caption = "Nara",
		.window_size = {WINDOWS_MIN_WIDTH * 4, WINDOWS_MIN_HEIGHT * 4},
		.window_min_size = {WINDOWS_MIN_WIDTH, WINDOWS_MIN_HEIGHT},
		.scale_mode = SCALE_MODE_STRETCH,
		.fullscreen = false,
		.clean_color = {0.80, 0.82, 0.84}
	},
	&st);

	if (context == NULL)
		goto return_failure;

	// Resources
	struct TerrainOptions terrain_options = {0};

	terrain_options.heightmap_filename = "./resources/heightmap.sgi";
	terrain_options.colormap_filename = "./resources/colormap.sgi";
	terrain_options.width = 10000; // 10 km
	terrain_options.height = 10000;
	terrain_options.elevation = 1000; // 1 km

	if ((terrain_program = ProgramCreate((char*)g_terrain_vertex, (char*)g_terrain_fragment, &st)) == NULL ||
	    (terrain = TerrainCreate(terrain_options, &st)) == NULL)
		goto return_failure;

	projection = Matrix4Perspective(FOV, (float)WINDOWS_MIN_WIDTH / (float)WINDOWS_MIN_HEIGHT, 0.1, 20000.0);

	ContextSetProjection(context, projection);
	ContextSetProgram(context, terrain_program);
	ContextSetCamera(context, camera_target,
	                 Vector3Add(camera_target, (struct Vector3){camera_distance, camera_distance, camera_distance}));

	// Yay!
	while (1)
	{
		ContextUpdate(context, &window_specs, &time_specs, &input_specs);
		ContextDraw(context, terrain->vertices, terrain->index, terrain->color);
		sCameraMove(context, &camera_target, &camera_distance);

		/*printf("%s %s %s %s %s %s %s %s %s %s %s\n", evn.a ? "a" : "-", evn.b ? "b" : "-", evn.x ? "x" : "-",
		       evn.y ? "y" : "-", evn.lb ? "lb" : "--", evn.rb ? "rb" : "--", evn.view ? "view" : "----",
		       evn.menu ? "menu" : "----", evn.guide ? "guide" : "-----", evn.ls ? "ls" : "--", evn.rs ? "rs" : "--");

		printf("left [x: %+0.2f, y: %+0.2f, t: %+0.2f], right [x: %+0.2f, y: %+0.2f, t: %+0.2f]\n", evn.left_analog.h,
		       evn.left_analog.v, evn.left_analog.t, evn.right_analog.h, evn.right_analog.v, evn.right_analog.t);

		printf("pad [x: %+0.2f, y: %+0.2f]\n", evn.pad.h, evn.pad.v);*/

		// Window specifications
		if (window_specs.resized == true)
		{
			printf("Window resized: %ix%i px\n", window_specs.size.x, window_specs.size.y);

			projection =
			    Matrix4Perspective(FOV, (float)window_specs.size.x / (float)window_specs.size.y, 0.1, 20000.0); // 20 km
			ContextSetProjection(context, projection);
		}

		if (window_specs.close == true)
			break;
	}

	// Bye!
	ProgramDelete(terrain_program);
	TerrainDelete(terrain);
	ContextDelete(context);

	return EXIT_SUCCESS;

return_failure:

	StatusPrint("Nara", st);
	if (terrain != NULL)
		TerrainDelete(terrain);
	if (terrain_program != NULL)
		ProgramDelete(terrain_program);
	if (context != NULL)
		ContextDelete(context);

	return EXIT_FAILURE;
}
