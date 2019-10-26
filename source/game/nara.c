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
#include "utilities.h"

#define FOV 45

#define WINDOWS_WIDTH 1152
#define WINDOWS_HEIGHT 480

#define WINDOWS_MIN_WIDTH 240
#define WINDOWS_MIN_HEIGHT 100

extern const uint8_t g_terrain_vertex[];
extern const uint8_t g_terrain_fragment[];
extern const uint8_t g_red_vertex[];
extern const uint8_t g_red_fragment[];

static struct Context* s_context = NULL;
static struct ContextEvents s_events;
static struct Mixer* s_mixer = NULL;
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

	projection = Matrix4Perspective(FOV, (float)window_size.x / (float)window_size.y, 0.1f, 20000.0f); // 20 km
	SetProjection(s_context, projection);
}


/*-----------------------------

 main()
-----------------------------*/
static bool sSingleClick(bool evn, bool* state)
{
	if (evn == false)
		*state = true;
	else if (*state == true)
	{
		*state = false;
		return true;
	}

	return false;
}

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

	// Initialization
	s_context = ContextCreate((struct ContextOptions){
		.caption = "Nara",
		.window_size = {WINDOWS_WIDTH, WINDOWS_HEIGHT},
		.window_min_size = {WINDOWS_MIN_WIDTH, WINDOWS_MIN_HEIGHT},
		.fullscreen = false,
		.clean_color = {0.80f, 0.82f, 0.84f}},
	&st);

	if (s_context == NULL)
		goto return_failure;

	s_mixer = MixerCreate((struct MixerOptions){
		.frequency = 48000,
		.channels = 2,
		.volume = 0.8f},
	&st);

	if (s_mixer == NULL)
		goto return_failure;

	TimerInit(&s_timer);

	// Resources
	{
		if ((terrain = NTerrainCreate("./assets/heightmap.sgi", 75.0, 972.0, 36.0, 3, &st)) == NULL)
			goto return_failure;

		if (ProgramInit(&terrain_program, (char*)g_terrain_vertex, (char*)g_terrain_fragment, &st) != 0)
			goto return_failure;

		if (TextureInit(&terrain_diffuse, "./assets/colormap.sgi", FILTER_TRILINEAR, &st) != 0)
			goto return_failure;

		if (SampleCreate(s_mixer, "./assets/rz1-closed-hithat.wav", &st) == NULL ||
		    SampleCreate(s_mixer, "./assets/rz1-clap.wav", &st) == NULL)
			goto return_failure;

		classes = sInitializeClasses();
		EntityCreate(&entities, ClassGet(classes, "Point"));

		camera_entity = EntityCreate(&entities, ClassGet(classes, "Camera"));
		camera_entity->co.position = (struct Vector3){128.0f, 128.0f, 256.0f};
		camera_entity->co.angle = (struct Vector3){-50.0f, 0.0f, 45.0f};

		sSetProjection((struct Vector2i){WINDOWS_WIDTH, WINDOWS_HEIGHT}, s_context);
	}

	NTerrainPrintInfo(terrain);

	// Game loop
	bool a_release = false;
	bool b_release = false;
	bool x_release = false;
	bool y_release = false;

	int draw_calls = 0;
	char title[128];

	Play2d(s_mixer, 0.7f, PLAY_LOOP, "./assets/ambient01.au");

	while (1)
	{
		TimerStep(&s_timer);
		ContextUpdate(s_context, &s_events);

		memcpy(&entities_input, &s_events, sizeof(struct EntityInput)); // HACK!
		entities_input.delta = (float)s_timer.miliseconds_betwen / 33.3333f;

		EntitiesUpdate(&entities, entities_input);

		// Update view
		if (Vector3Equals(camera_entity->co.position, camera_entity->old_co.position) == false ||
		    Vector3Equals(camera_entity->co.angle, camera_entity->old_co.angle) == false)
		{
			camera_matrix = Matrix4Identity();
			camera_matrix = Matrix4RotateX(camera_matrix, DegToRad(camera_entity->co.angle.x));
			camera_matrix = Matrix4RotateZ(camera_matrix, DegToRad(camera_entity->co.angle.z));
			camera_matrix = Matrix4Multiply(camera_matrix, Matrix4Translate(Vector3Invert(camera_entity->co.position)));

			SetCamera(s_context, camera_matrix, camera_entity->co.position);
		}

		if (s_events.resized == true)
			sSetProjection(s_events.window_size, s_context);

		// Render
		SetProgram(s_context, &terrain_program);
		SetDiffuse(s_context, &terrain_diffuse);

		draw_calls = NTerrainDraw(terrain, 1024.0f, camera_entity->co.position, camera_entity->co.angle);

		if (s_timer.one_second == true)
		{
			sprintf(title, "Nara v0.1 | dcalls: %i, fps: %i (~%.02f)", draw_calls, s_timer.frames_per_second,
			        s_timer.miliseconds_betwen);
			SetTitle(s_context, title);
		}

		// Testing, testing
		if (sSingleClick(s_events.a, &a_release) == true)
			Play2d(s_mixer, 1.0f, PLAY_NORMAL, "./assets/rz1-kick.wav");

		if (sSingleClick(s_events.b, &b_release) == true)
			Play2d(s_mixer, 1.0f, PLAY_NORMAL, "./assets/rz1-snare.wav");

		if (sSingleClick(s_events.x, &x_release) == true)
			Play2d(s_mixer, 1.0f, PLAY_NORMAL, "./assets/rz1-closed-hithat.wav");

		if (sSingleClick(s_events.y, &y_release) == true)
			Play2d(s_mixer, 1.0f, PLAY_NORMAL, "./assets/rz1-clap.wav");

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

	MixerDelete(s_mixer);
	ContextDelete(s_context);

	return EXIT_SUCCESS;

return_failure:
	StatusPrint("Nara", st);
	return EXIT_FAILURE;
}
