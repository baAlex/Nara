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

#include "../engine/context.h"
#include "../engine/entity.h"
#include "../engine/mixer.h"
#include "../engine/nterrain.h"
#include "../engine/timer.h"

#include "game.h"

#define FOV 45

#define WINDOWS_MIN_WIDTH 220
#define WINDOWS_MIN_HEIGHT 100

extern const uint8_t g_terrain_vertex[];
extern const uint8_t g_terrain_fragment[];
extern const uint8_t g_red_vertex[];
extern const uint8_t g_red_fragment[];

static struct Context* s_context = NULL;
static struct ContextEvents s_events;
static struct Timer s_timer;


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

 main()
-----------------------------*/
int main()
{
	struct Status st = {0};

	struct NTerrain* terrain = NULL;
	struct Program terrain_program = {0};
	struct Texture terrain_diffuse = {0};

	struct Dictionary* classes = NULL;
	struct List entities = {0};
	struct EntityInput entities_input = {0};
	struct Entity* camera_entity = NULL;

	struct Matrix4 camera_matrix = Matrix4Identity();

	printf("Nara v0.1\n");

	// Context initialization
	s_context = ContextCreate((struct ContextOptions)
	{
		.caption = "Nara",
		.window_size = {WINDOWS_MIN_WIDTH * 4, WINDOWS_MIN_HEIGHT * 4},
		.window_min_size = {WINDOWS_MIN_WIDTH, WINDOWS_MIN_HEIGHT},
		.fullscreen = false,
		.clean_color = {0.80, 0.82, 0.84}
	}, &st);

	if (s_context == NULL)
		goto return_failure;

	TimerInit(&s_timer);

	// Resources
	{
		if ((terrain = NTerrainCreate("./assets/heightmap.sgi", 75.0, 972.0, 18.0, 3, &st)) == NULL)
			goto return_failure;

		if (ProgramInit(&terrain_program, (char*)g_terrain_vertex, (char*)g_terrain_fragment, &st) != 0)
			goto return_failure;

		if (TextureInit(&terrain_diffuse, "./assets/colormap.sgi", FILTER_BILINEAR, &st) != 0)
			goto return_failure;

		classes = sInitializeClasses();
		EntityCreate(&entities, ClassGet(classes, "Point"));

		camera_entity = EntityCreate(&entities, ClassGet(classes, "Camera"));
		camera_entity->co.position = (struct Vector3){128.0f, 128.0f, 256.0f};
		camera_entity->co.angle = (struct Vector3){-50.0f, 0.0f, 45.0f};

		sSetProjection((struct Vector2i){WINDOWS_MIN_WIDTH, WINDOWS_MIN_HEIGHT}, s_context);
	}

	printf("Terrain 0x%p:\n", (void*)terrain);
	printf(" - Dimension: %0.4f\n", terrain->dimension);
	printf(" - Minimum tile dimension: %0.4f\n", terrain->min_tile_dimension);
	printf(" - Minimum pattern dimension: %0.4f\n", terrain->min_pattern_dimension);
	printf(" - Steps: %lu\n", terrain->steps);
	printf(" - Tiles: %lu\n", terrain->tiles_no);
	printf(" - Vertices buffers: %lu\n", terrain->vertices_buffers_no);

	// Game loop
	while (1)
	{
		TimerStep(&s_timer);
		ContextUpdate(s_context, &s_events);

		memcpy(&entities_input, &s_events, sizeof(struct EntityInput)); // HACK!
		entities_input.delta = s_timer.miliseconds_betwen / 33.3333;

		EntitiesUpdate(&entities, entities_input);

		// Update view
		if (Vector3Equals(camera_entity->co.position, camera_entity->old_co.position) == false ||
		    Vector3Equals(camera_entity->co.angle, camera_entity->old_co.angle) == false)
		{
			camera_matrix = Matrix4Identity();
			camera_matrix = Matrix4RotateX(camera_matrix, DEG_TO_RAD(camera_entity->co.angle.x));
			camera_matrix = Matrix4RotateZ(camera_matrix, DEG_TO_RAD(camera_entity->co.angle.z));
			camera_matrix = Matrix4Multiply(camera_matrix, Matrix4Translate(Vector3Invert(camera_entity->co.position)));

			ContextSetCamera(s_context, camera_matrix, camera_entity->co.position);
		}

		if (s_events.resized == true)
			sSetProjection(s_events.window_size, s_context);

		// Render
		ContextSetProgram(s_context, &terrain_program);
		ContextSetDiffuse(s_context, &terrain_diffuse);
		NTerrainDraw(terrain, camera_entity->co.position);

		// Exit?
		if (s_events.close == true)
			break;
	}

	// Bye!
	DictionaryDelete(classes);
	ListClean(&entities);
	ProgramFree(&terrain_program);
	TextureFree(&terrain_diffuse);
	NTerrainDelete(terrain);
	ContextDelete(s_context);

	return EXIT_SUCCESS;

return_failure:

	StatusPrint("Nara", st);
	if (terrain != NULL)
		NTerrainDelete(terrain);
	if (terrain_diffuse.glptr != 0)
		TextureFree(&terrain_diffuse);
	if (terrain_program.glptr != 0)
		ProgramFree(&terrain_program);
	if (s_context != NULL)
		ContextDelete(s_context);

	return EXIT_FAILURE;
}
