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


#define NAME "Nara v0.2-alpha"
#define NAME_SHORT "Nara"

#define FOV 45

#define WINDOWS_WIDTH 1152
#define WINDOWS_HEIGHT 480
#define WINDOWS_MIN_WIDTH 240
#define WINDOWS_MIN_HEIGHT 100


extern const uint8_t g_terrain_vertex[];
extern const uint8_t g_terrain_fragment[];

static struct Context* s_context = NULL;
static struct Mixer* s_mixer = NULL;
static struct Timer s_timer = {0};

static struct NTerrain* s_terrain = NULL;
static struct Program s_terrain_program = {0};
static struct Texture s_terrain_texture = {0};

static struct Dictionary* s_classes = NULL;
static struct List s_entities = {0};


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
static void sSetProjection(struct Vector2i window_size, struct Context* context)
{
	struct Matrix4 projection;

	projection = Matrix4Perspective(FOV, (float)window_size.x / (float)window_size.y, 0.1f, 20000.0f);
	SetProjection(context, projection);
}


/*-----------------------------

 sSingleClick()
-----------------------------*/
static inline bool sSingleClick(bool evn, bool* state)
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


/*-----------------------------

 main()
-----------------------------*/
int main()
{
	struct Status st = {0};
	struct Entity* camera = NULL;

	printf("%s\n", NAME);

	// Engine initialization
	{
		struct ContextOptions context_options = {0};
		struct MixerOptions mixer_options = {0};

		context_options.caption = NAME_SHORT;
		context_options.window_size = (struct Vector2i){WINDOWS_WIDTH, WINDOWS_HEIGHT};
		context_options.window_min_size = (struct Vector2i){WINDOWS_MIN_WIDTH, WINDOWS_MIN_HEIGHT};
		context_options.clean_color = (struct Vector3){0.80f, 0.82f, 0.84f};
		context_options.fullscreen = false;
		context_options.disable_vsync = false;
		context_options.samples = 2;

		mixer_options.frequency = 48000;
		mixer_options.channels = 2;
		mixer_options.volume = 0.8f;

		if ((s_context = ContextCreate(context_options, &st)) == NULL)
			goto return_failure;

		if ((s_mixer = MixerCreate(mixer_options, &st)) == NULL)
			goto return_failure;

		TimerInit(&s_timer);
	}

	// Resources
	{
		if ((s_terrain = NTerrainCreate("./assets/heightmap.sgi", 75.0, 972.0, 36.0, 3, &st)) == NULL)
			goto return_failure;

		if (ProgramInit(&s_terrain_program, (char*)g_terrain_vertex, (char*)g_terrain_fragment, &st) != 0)
			goto return_failure;

		if (TextureInit(&s_terrain_texture, "./assets/colormap.sgi", FILTER_TRILINEAR, &st) != 0)
			goto return_failure;

		if (SampleCreate(s_mixer, "./assets/rz1-closed-hithat.wav", &st) == NULL)
			goto return_failure;

		if (SampleCreate(s_mixer, "./assets/rz1-clap.wav", &st) == NULL)
			goto return_failure;

		if (SampleCreate(s_mixer, "./assets/ambient01.au", &st) == NULL)
			goto return_failure;

		NTerrainPrintInfo(s_terrain);

		// Entities
		s_classes = sInitializeClasses();

		EntityCreate(&s_entities, ClassGet(s_classes, "Point"));
		camera = EntityCreate(&s_entities, ClassGet(s_classes, "Camera"));
		camera->co.position = (struct Vector3){128.0f, 128.0f, 256.0f};
		camera->co.angle = (struct Vector3){-50.0f, 0.0f, 45.0f};

		sSetProjection((struct Vector2i){WINDOWS_WIDTH, WINDOWS_HEIGHT}, s_context);
	}

	// Game loop
	{
		struct ContextEvents events = {0};
		struct EntityInput entities_input = {0};
		struct NTerrainView terrain_view = {0};
		struct Matrix4 matrix = {0};

		int draw_calls = 0;
		char title[128] = {0};

		bool a_release = false;
		bool b_release = false;
		bool x_release = false;
		bool y_release = false;

		Play2d(s_mixer, 0.7f, PLAY_LOOP, "./assets/ambient01.au");
		SetProgram(s_context, &s_terrain_program);
		SetTexture(s_context, &s_terrain_texture);

		if (MixerStart(s_mixer, &st) != 0)
			goto return_failure;

		while (1)
		{
			// Update engine
			TimerUpdate(&s_timer);
			ContextUpdate(s_context, &events);

			// Update entities
			entities_input.a = events.a;
			entities_input.b = events.b;
			entities_input.x = events.x;
			entities_input.y = events.y;

			entities_input.lb = events.lb;
			entities_input.rb = events.rb;

			entities_input.view = events.view;
			entities_input.menu = events.menu;
			entities_input.guide = events.guide;

			entities_input.ls = events.ls;
			entities_input.rs = events.rs;

			entities_input.pad.h = events.pad.h;
			entities_input.pad.v = events.pad.v;

			entities_input.left_analog.h = events.left_analog.h;
			entities_input.left_analog.v = events.left_analog.v;
			entities_input.left_analog.t = events.left_analog.t;

			entities_input.right_analog.h = events.right_analog.h;
			entities_input.right_analog.v = events.right_analog.v;
			entities_input.right_analog.t = events.right_analog.t;

			entities_input.delta = (float)s_timer.miliseconds_betwen / 33.3333f;

			EntitiesUpdate(&s_entities, entities_input);

			// Update view
			if (Vector3Equals(camera->co.position, camera->old_co.position) == false ||
			    Vector3Equals(camera->co.angle, camera->old_co.angle) == false)
			{
				matrix = Matrix4Identity();
				matrix = Matrix4RotateX(matrix, DegToRad(camera->co.angle.x));
				matrix = Matrix4RotateZ(matrix, DegToRad(camera->co.angle.z));
				matrix = Matrix4Multiply(matrix, Matrix4Translate(Vector3Invert(camera->co.position)));

				SetCamera(s_context, matrix, camera->co.position);
			}

			if (events.resized == true)
				sSetProjection(events.window_size, s_context);

			// Render
			terrain_view.angle = camera->co.angle;
			terrain_view.position = camera->co.position;
			terrain_view.max_distance = 1024.0f;

			draw_calls = NTerrainDraw(s_context, s_terrain, &terrain_view);

			if (s_timer.one_second == true)
			{
				sprintf(title, "%s | dcalls: %i, fps: %i (~%.02f)", NAME, draw_calls, s_timer.frames_per_second,
				        s_timer.miliseconds_betwen);
				SetTitle(s_context, title);
			}

			// Testing, testing
			if (sSingleClick(events.a, &a_release) == true)
				Play2d(s_mixer, 1.0f, PLAY_NORMAL, "./assets/rz1-kick.wav");

			if (sSingleClick(events.b, &b_release) == true)
				Play2d(s_mixer, 1.0f, PLAY_NORMAL, "./assets/rz1-snare.wav");

			if (sSingleClick(events.x, &x_release) == true)
				Play2d(s_mixer, 1.0f, PLAY_NORMAL, "./assets/rz1-closed-hithat.wav");

			if (sSingleClick(events.y, &y_release) == true)
				Play2d(s_mixer, 1.0f, PLAY_NORMAL, "./assets/rz1-clap.wav");

			// Exit?
			if (events.close == true)
				break;
		}
	}

	// Bye!
	DictionaryDelete(s_classes);
	ListClean(&s_entities);

	ProgramFree(&s_terrain_program);
	TextureFree(&s_terrain_texture);
	NTerrainDelete(s_terrain);

	MixerDelete(s_mixer);
	ContextDelete(s_context);

	return EXIT_SUCCESS;

return_failure:
	StatusPrint(NAME_SHORT, st);
	return EXIT_FAILURE;
}
