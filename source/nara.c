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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "context.h"

#include "shaders/white-fragment.h"
#include "shaders/white-vertex.h"

#define M_PI 3.14159265358979323846264338327950288
#define DEG_TO_RAD(d) ((d) * M_PI / 180.0)
#define RAD_TO_DEG(r) ((r) * 180.0 / M_PI)

#define FOV 45

// For test purposes I using an aspect of 2.20:1
// In real world the aspect should not be enforced
#define CANVAS_WIDTH 528
#define CANVAS_HEIGHT 240
#define WINDOWS_MIN_WIDTH 220
#define WINDOWS_MIN_HEIGHT 100


const struct Vertex g_test_vertices[] = {
	{.pos = {-0.5, -0.5, 0.0}, .nor = {0.0, 0.0, 0.0}},
	{.pos = {0.5, -0.5, 0.0}, .nor = {0.0, 0.0, 0.0}},
	{.pos = {0.5, 0.5, 0.0}, .nor = {0.0, 0.0, 0.0}},
	{.pos = {-0.5, 0.5, 0.0}, .nor = {0.0, 0.0, 0.0}},

	{.pos = {0.0, 0.0, 1.0}, .nor = {0.0, 0.0, 0.0}},
	{.pos = {1.0, 0.0, 1.0}, .nor = {0.0, 0.0, 0.0}},
	{.pos = {1.0, 1.0, 1.0}, .nor = {0.0, 0.0, 0.0}},
	{.pos = {0.0, 1.0, 1.0}, .nor = {0.0, 0.0, 0.0}}};

const uint16_t g_test_index[] = {
	0, 1, 2,
	0, 3, 2,
};


/*-----------------------------

 sCameraMove()
-----------------------------*/
static void sCameraMove(struct Context* context, struct Events* evn, struct Vector3* target, float* distance)
{
	// The following lazy code should work
	// until introduction of analog input

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

	struct Program* white_program = NULL;
	struct Vertices* test_vertices = NULL;
	struct Index* test_index = NULL;

	struct Matrix4 projection;
	struct Vector3 camera_target = {0.0, 0.0, 0.0};
	float camera_distance = 5.0;

	printf("Nara v0.1\n");
	printf("- Lib-Japan v%i.%i.%i\n", JAPAN_VERSION_MAJOR, JAPAN_VERSION_MINOR, JAPAN_VERSION_PATCH);
	printf("\n");

	// Initialization
	context = ContextCreate((struct ContextOptions)
	{
		.caption = "Nara",
		.window_size = {CANVAS_WIDTH, CANVAS_HEIGHT},
		.windows_min_size = {WINDOWS_MIN_WIDTH, WINDOWS_MIN_HEIGHT},
		.aspect = {WINDOWS_MIN_WIDTH, WINDOWS_MIN_HEIGHT},
		.scale_mode = SCALE_MODE_ASPECT,
		.fullscreen = false,
		.clean_color = {0.0, 0.0, 0.0}

	}, &st);

	if (context == NULL)
		goto return_failure;

	// Resources
	if ((white_program = ProgramCreate((char*)g_white_vertex, (char*)g_white_fragment, &st)) == NULL ||
		(test_vertices = VerticesCreate(g_test_vertices, 8, &st)) == NULL ||
		(test_index = IndexCreate(g_test_index, 6, &st)) == NULL)
		goto return_failure;

	projection = Matrix4Perspective(FOV, (float)CANVAS_WIDTH / (float)CANVAS_HEIGHT, 0.1, 100.0);

	ContextSetProjection(context, projection);
	ContextSetProgram(context, white_program);
	ContextSetCamera(context, camera_target, Vector3Add(camera_target, (struct Vector3)
	{
		camera_distance, camera_distance, camera_distance
	}));

	// Yay!
	while (1)
	{
		ContextUpdate(context, &evn);
		ContextDraw(context, test_vertices, test_index);

		// Events
		/*printf("%s %s %s %s %s %s %s %s %s %s %s\n", evn.a ? "a" : "-", evn.b ? "b" : "-", evn.x ? "x" : "-",
			   evn.y ? "y" : "-", evn.lb ? "lb" : "--", evn.rb ? "rb" : "--", evn.view ? "view" : "----",
			   evn.menu ? "menu" : "----", evn.guide ? "guide" : "-----", evn.ls ? "ls" : "--", evn.rs ? "rs" : "--");*/

		sCameraMove(context, &evn, &camera_target, &camera_distance);

		if (evn.system.exit == true)
			break;
	}

	// Bye!
	ContextDelete(context);
	return EXIT_SUCCESS;

return_failure:

	StatusPrint(st);
	if (test_index != NULL)
		IndexDelete(test_index);
	if (test_vertices != NULL)
		VerticesDelete(test_vertices);
	if (white_program != NULL)
		ProgramDelete(white_program);
	if (context != NULL)
		ContextDelete(context);

	return EXIT_FAILURE;
}
