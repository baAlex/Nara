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

#include "../game/game.h"

#define FOV 45

#define WINDOWS_MIN_WIDTH 220
#define WINDOWS_MIN_HEIGHT 100

extern const uint8_t g_terrain_vertex[];
extern const uint8_t g_terrain_fragment[];
extern const uint8_t g_red_vertex[];
extern const uint8_t g_red_fragment[];

static struct WindowSpecifications s_window_specs;
static struct TimeSpecifications s_time_specs;
static struct InputSpecifications s_input_specs;
static struct Context* s_context = NULL;


/*-----------------------------

 sInitializeClasses()
-----------------------------*/
static struct Dictionary* sInitializeClasses()
{
	struct Dictionary* classes = DictionaryCreate(NULL);
	struct Class* class = NULL;

	class = ClassCreate(classes, "Camera");
	class->func_start = GameCameraStart;
	class->func_think = GameCameraThink;
	class->func_delete = GameCameraDelete;

	class = ClassCreate(classes, "Point");
	class->func_start = GamePointStart;
	class->func_think = GamePointThink;
	class->func_delete = GamePointDelete;

	return classes;
}


/*-----------------------------

 sSetProjection()
-----------------------------*/
static void sSetProjection(struct Vector2i window_size, struct Context* s_context)
{
	struct Matrix4 projection;

	projection = Matrix4Perspective(FOV, (float)window_size.x / (float)window_size.y, 0.1, 20000.0); // 20 km
	ContextSetProjection(s_context, projection);
}


/*-----------------------------

 sSetCamera()
-----------------------------*/
static void sSetCamera(const struct Entity* camera, struct Context* s_context)
{
	struct Matrix4 projection;
	struct Matrix4 translation;

	projection = Matrix4Identity();
	projection = Matrix4RotateX(projection, DEG_TO_RAD(camera->co.angle.x));
	projection = Matrix4RotateZ(projection, DEG_TO_RAD(camera->co.angle.z));

	translation = Matrix4Translate(Vector3Invert(camera->co.position));

	ContextSetCameraAsMatrix(s_context, Matrix4Multiply(projection, translation), camera->co.position);
}


/*-----------------------------

 main()
-----------------------------*/
int main()
{
	struct Status st = {0};

	struct List entities = {0};             // In a future these three should
	struct Program* terrain_program = NULL; // be component of an object representing
	struct Terrain* terrain = NULL;         // a game map/level. Being loaded from a file

	struct Dictionary* classes = sInitializeClasses(); // TODO: error check
	struct EntityInput entities_input = {0};
	struct Entity* camera_entity = NULL;


	printf("Nara v0.1\n");
	printf("- Lib-Japan v%i.%i.%i\n", JAPAN_VERSION_MAJOR, JAPAN_VERSION_MINOR, JAPAN_VERSION_PATCH);
	printf("\n");

	// Context initialization
	s_context = ContextCreate((struct ContextOptions){
		.caption = "Nara",
		.window_size = {WINDOWS_MIN_WIDTH * 4, WINDOWS_MIN_HEIGHT * 4},
		.window_min_size = {WINDOWS_MIN_WIDTH, WINDOWS_MIN_HEIGHT},
		.scale_mode = SCALE_MODE_STRETCH,
		.fullscreen = false,
		.clean_color = {0.80, 0.82, 0.84}
	},
	&st);

	if (s_context == NULL)
		goto return_failure;

	// Resources
	{
		struct TerrainOptions terrain_options = {0};

		terrain_options.heightmap_filename = "./resources/heightmap.sgi";
		terrain_options.colormap_filename = "./resources/colormap.sgi";
		terrain_options.width = 1024;
		terrain_options.height = 1024;
		terrain_options.elevation = 64;

		if ((terrain_program = ProgramCreate((char*)g_terrain_vertex, (char*)g_terrain_fragment, &st)) == NULL ||
		    (terrain = TerrainCreate(terrain_options, &st)) == NULL)
			goto return_failure;

		sSetProjection((struct Vector2i){WINDOWS_MIN_WIDTH, WINDOWS_MIN_HEIGHT}, s_context);
		ContextSetProgram(s_context, terrain_program);

		camera_entity = EntityCreate(&entities, ClassGet(classes, "Camera"));
		EntityCreate(&entities, ClassGet(classes, "Point"));
	}

	// Yay!
	while (1)
	{
		ContextUpdate(s_context, &s_window_specs, &s_time_specs, &s_input_specs);
		ContextDraw(s_context, terrain->vertices, terrain->index, terrain->color);

		memcpy(&entities_input, &s_input_specs, sizeof(struct InputSpecifications)); // HACK!
		entities_input.delta = s_time_specs.miliseconds_betwen / 33.3333;

		EntitiesUpdate(&entities, entities_input);

		/*printf("%s %s %s %s %s %s %s %s %s %s %s\n", s_input_specs.a ? "a" : "-", s_input_specs.b ? "b" : "-",
		s_input_specs.x ? "x" : "-", s_input_specs.y ? "y" : "-", s_input_specs.lb ? "lb" : "--", s_input_specs.rb ?
		"rb" :
		"--", s_input_specs.view ? "view" : "----", s_input_specs.menu ? "menu" : "----", s_input_specs.guide ? "guide"
		:
		"-----", s_input_specs.ls ? "ls" : "--", s_input_specs.rs ? "rs" : "--");

		printf("left [x: %+0.2f, y: %+0.2f, t: %+0.2f], right [x: %+0.2f, y: %+0.2f, t: %+0.2f]\n",
		s_input_specs.left_analog.h, s_input_specs.left_analog.v, s_input_specs.left_analog.t,
		s_input_specs.right_analog.h, s_input_specs.right_analog.v, s_input_specs.right_analog.t);

		printf("pad [x: %+0.2f, y: %+0.2f]\n", s_input_specs.pad.h, s_input_specs.pad.v);*/

		// Update view
		if (Vector3Equals(camera_entity->co.position, camera_entity->old_co.position) == false ||
		    Vector3Equals(camera_entity->co.angle, camera_entity->old_co.angle) == false)
		{
			sSetCamera(camera_entity, s_context);
		}

		if (s_window_specs.resized == true)
			sSetProjection(s_window_specs.size, s_context);

		// Exit?
		if (s_window_specs.close == true)
			break;
	}

	// Bye!
	DictionaryDelete(classes);
	ListClean(&entities);
	ProgramDelete(terrain_program);
	TerrainDelete(terrain);
	ContextDelete(s_context);

	return EXIT_SUCCESS;

return_failure:

	StatusPrint("Nara", st);
	if (terrain != NULL)
		TerrainDelete(terrain);
	if (terrain_program != NULL)
		ProgramDelete(terrain_program);
	if (s_context != NULL)
		ContextDelete(s_context);

	return EXIT_FAILURE;
}