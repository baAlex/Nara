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
#include "entity.h"
#include "terrain.h"
#include "game.h"

#define FOV 45

#define WINDOWS_MIN_WIDTH 220
#define WINDOWS_MIN_HEIGHT 100

extern const uint8_t g_terrain_vertex[];
extern const uint8_t g_terrain_fragment[];
extern const uint8_t g_red_vertex[];
extern const uint8_t g_red_fragment[];

struct WindowSpecifications g_window_specs;
struct TimeSpecifications g_time_specs;
struct InputSpecifications g_input_specs;
struct Context* g_context = NULL;


/*-----------------------------

 main()
-----------------------------*/
int main()
{
	struct Status st = {0};
	struct Dictionary* class_definitions = NULL;

	struct List entities = {0};             // In a future these three should
	struct Program* terrain_program = NULL; // be component of an object representing
	struct Terrain* terrain = NULL;         // a game map/level. Being loaded from a file

	struct Matrix4 projection;
	struct Entity* camera = NULL;

	printf("Nara v0.1\n");
	printf("- Lib-Japan v%i.%i.%i\n", JAPAN_VERSION_MAJOR, JAPAN_VERSION_MINOR, JAPAN_VERSION_PATCH);
	printf("\n");

	// Context initialization
	g_context = ContextCreate((struct ContextOptions){
		.caption = "Nara",
		.window_size = {WINDOWS_MIN_WIDTH * 4, WINDOWS_MIN_HEIGHT * 4},
		.window_min_size = {WINDOWS_MIN_WIDTH, WINDOWS_MIN_HEIGHT},
		.scale_mode = SCALE_MODE_STRETCH,
		.fullscreen = false,
		.clean_color = {0.80, 0.82, 0.84}
	},
	&st);

	if (g_context == NULL)
		goto return_failure;

	// Classes initialization
	{
		class_definitions = DictionaryCreate(NULL);
		struct Class* class = NULL;

		class = ClassCreate(class_definitions, "Camera");
		class->entity_data_size = sizeof(struct GameCamera);
		class->class_data_size = 0;
		class->func_start = GameCameraStart;
		class->func_think = GameCameraThink;
		class->func_delete = GameCameraDelete;

		class = ClassCreate(class_definitions, "Player");
		class->entity_data_size = sizeof(struct GamePlayer);
		class->class_data_size = 0;
		class->func_start = GamePlayerStart;
		class->func_think = GamePlayerThink;
		class->func_delete = GamePlayerDelete;
	}

	// Resources
	{
		struct TerrainOptions terrain_options = {0};

		terrain_options.heightmap_filename = "./resources/heightmap.sgi";
		terrain_options.colormap_filename = "./resources/colormap.sgi";
		terrain_options.width = 10000; // 10 km
		terrain_options.height = 10000;
		terrain_options.elevation = 1000; // 1 km

		if ((terrain_program = ProgramCreate((char*)g_terrain_vertex, (char*)g_terrain_fragment, &st)) == NULL ||
		    (terrain = TerrainCreate(terrain_options, &st)) == NULL)
			goto return_failure;

		projection = Matrix4Perspective(FOV, (float)WINDOWS_MIN_WIDTH / (float)WINDOWS_MIN_HEIGHT, 0.1, 20000.0); // 20 km
		ContextSetProjection(g_context, projection);
		ContextSetProgram(g_context, terrain_program);

		// Entities
		camera = EntityCreate(&entities, ClassGet(class_definitions, "Camera"), (struct Vector3){4.0, 5.0, 6.0});
		EntityCreate(&entities, ClassGet(class_definitions, "Player"), (struct Vector3){1.0, 2.0, 3.0});
	}

	// Yay!
	while (1)
	{
		ContextUpdate(g_context, &g_window_specs, &g_time_specs, &g_input_specs);
		ContextDraw(g_context, terrain->vertices, terrain->index, terrain->color);

		EntitiesUpdate(&entities, (g_time_specs.miliseconds_betwen / 33.3333));

		/*printf("%s %s %s %s %s %s %s %s %s %s %s\n", g_input_specs.a ? "a" : "-", g_input_specs.b ? "b" : "-",
		g_input_specs.x ? "x" : "-", g_input_specs.y ? "y" : "-", g_input_specs.lb ? "lb" : "--", g_input_specs.rb ? "rb" :
		"--", g_input_specs.view ? "view" : "----", g_input_specs.menu ? "menu" : "----", g_input_specs.guide ? "guide" :
		"-----", g_input_specs.ls ? "ls" : "--", g_input_specs.rs ? "rs" : "--");

		printf("left [x: %+0.2f, y: %+0.2f, t: %+0.2f], right [x: %+0.2f, y: %+0.2f, t: %+0.2f]\n",
		g_input_specs.left_analog.h, g_input_specs.left_analog.v, g_input_specs.left_analog.t, g_input_specs.right_analog.h,
		g_input_specs.right_analog.v, g_input_specs.right_analog.t);

		printf("pad [x: %+0.2f, y: %+0.2f]\n", g_input_specs.pad.h, g_input_specs.pad.v);*/

		// Update view
		if (Vector3Equals(camera->position, camera->old_position) == false
		|| Vector3Equals(camera->angle, camera->old_angle) == false)
		{
			struct Matrix4 rot;
			struct Matrix4 pos;

			rot = Matrix4Identity();
			rot = Matrix4RotateX(rot, DEG_TO_RAD(camera->angle.x));
			rot = Matrix4RotateZ(rot, DEG_TO_RAD(camera->angle.z));

			pos = Matrix4Translate(Vector3Invert(camera->position));

			ContextSetCameraAsMatrix(g_context, Matrix4Multiply(rot, pos));
		}

		if (g_window_specs.resized == true)
		{
			printf("Window resized: %ix%i px\n", g_window_specs.size.x, g_window_specs.size.y);

			projection =
			    Matrix4Perspective(FOV, (float)g_window_specs.size.x / (float)g_window_specs.size.y, 0.1, 20000.0); // 20 km
			ContextSetProjection(g_context, projection);
		}

		// Exit?
		if (g_window_specs.close == true)
			break;
	}

	// Bye!
	DictionaryDelete(class_definitions);
	ListClean(&entities);
	ProgramDelete(terrain_program);
	TerrainDelete(terrain);
	ContextDelete(g_context);

	return EXIT_SUCCESS;

return_failure:

	StatusPrint("Nara", st);
	if (terrain != NULL)
		TerrainDelete(terrain);
	if (terrain_program != NULL)
		ProgramDelete(terrain_program);
	if (g_context != NULL)
		ContextDelete(g_context);

	return EXIT_FAILURE;
}
