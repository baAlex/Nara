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

#include "context.h"

#include "shaders/white-fragment.h"
#include "shaders/white-vertex.h"

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
	struct Matrix4 camera;

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
	camera = Matrix4LookAt((struct Vector3){5.0, 5.0, 5.0},
	                       (struct Vector3){0.0, 0.0, 0.0},
	                       (struct Vector3){0.0, 0.0, 1.0});

	ContextSetProjection(context, projection);
	ContextSetCamera(context, camera);

	ContextSetProgram(context, white_program);

	// Yay!
	while (1)
	{
		ContextUpdate(context, &evn);
		ContextDraw(context, test_vertices, test_index);

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
