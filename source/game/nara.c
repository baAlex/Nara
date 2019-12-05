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
#include "../engine/options.h"
#include "../engine/timer.h"

#include "game.h"
#include "utilities.h"


#define NAME "Nara v0.2-alpha"
#define NAME_SHORT "Nara"

#define FOV 75.0f

#define WINDOWS_WIDTH 1200
#define WINDOWS_HEIGHT 600
#define WINDOWS_MIN_WIDTH 200
#define WINDOWS_MIN_HEIGHT 100


extern const uint8_t g_terrain_vertex[];
extern const uint8_t g_terrain_fragment[];

static struct Context* s_context = NULL;
static struct Mixer* s_mixer = NULL;
static struct Timer s_timer = {0};

static struct NTerrain* s_terrain = NULL;
static struct NTerrainView s_terrain_view = {0};
static struct Program s_terrain_program = {0};
static struct Texture s_terrain_normalmap = {0};

static struct Texture s_cliff1 = {0};
static struct Texture s_cliff2 = {0};
static struct Texture s_grass = {0};
static struct Texture s_dirt = {0};

static struct Dictionary* s_classes = NULL;
static struct List s_entities = {0};

static struct Options* s_options = NULL;


/*-----------------------------

 sInitializeOptions()
-----------------------------*/
static struct Options* sInitializeOptions(int argc, const char* argv[])
{
	// TODO, check errors
	struct Options* options = OptionsCreate();

	if (options != NULL)
	{
		OptionsRegister(options, "r_width", 1200, NULL);
		OptionsRegister(options, "r_height", 600, NULL);
		OptionsRegister(options, "r_fullscreen", false, NULL);
		OptionsRegister(options, "r_vsync", true, NULL);
		OptionsRegister(options, "r_samples", 2, NULL);
		OptionsRegister(options, "r_max_distance", 1024, NULL);
		OptionsRegister(options, "r_wireframe", false, NULL);
		OptionsRegister(options, "r_filter", "trilinear", NULL);

		OptionsRegister(options, "s_volume", 0.8f, NULL);
		OptionsRegister(options, "s_frequency", 48000, NULL);
		OptionsRegister(options, "s_channels", 2, NULL);
		OptionsRegister(options, "s_max_sounds", 32, NULL);
		OptionsRegister(options, "s_sampling", "sinc_medium", NULL);

		OptionsRegister(options, "terrain_subvidisions", 3, NULL);
		OptionsRegister(options, "terrain_lod_factor", 0, NULL);

		OptionsRegister(options, "sensitivity", 1.0f, NULL);
		OptionsRegister(options, "fov", 90, NULL);
	}

	OptionsReadArguments(options, argc, argv, NULL);
	OptionsReadFile(options, "user.cfg", NULL);

	return options;
}


/*-----------------------------

 sInitializeClasses()
-----------------------------*/
static struct Dictionary* sInitializeClasses()
{
	// TODO, check errors
	struct Dictionary* classes = DictionaryCreate(NULL);
	struct Class* class = NULL;

	if (classes != NULL)
	{
		class = ClassCreate(classes, "Camera");
		class->func_start = GameCameraStart;
		class->func_think = GameCameraThink;
		class->func_delete = GameCameraDelete;

		class = ClassCreate(classes, "Point");
		class->func_start = GamePointStart;
		class->func_think = GamePointThink;
		class->func_delete = GamePointDelete;
	}

	return classes;
}


/*-----------------------------

 sSetProjection()
-----------------------------*/
static void sSetProjection(struct Vector2i window_size, struct Context* context)
{
	struct Matrix4 projection;

	projection = Matrix4Perspective(DegToRad(FOV), (float)window_size.x / (float)window_size.y, 0.1f, 1024.0f);
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
int main(int argc, const char* argv[])
{
	struct Status st = {0};
	struct Entity* camera = NULL;

	printf("%s\n", NAME);

	if ((s_options = sInitializeOptions(argc, argv)) == NULL)
	{
		StatusSet(&st, "sInitializeOptions", STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

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
		if ((s_terrain = NTerrainCreate("./assets/heightmap.sgi", 150.0, 1944.0, 72.0, 3, &st)) == NULL)
			goto return_failure;

		if (ProgramInit(&s_terrain_program, (char*)g_terrain_vertex, (char*)g_terrain_fragment, &st) != 0)
			goto return_failure;

		if (TextureInit(&s_terrain_normalmap, "./assets/normalmap.sgi", FILTER_TRILINEAR, &st) != 0)
			goto return_failure;

		if (TextureInit(&s_cliff1, "./assets/cliff1.sgi", FILTER_TRILINEAR, &st) != 0)
			goto return_failure;

		if (TextureInit(&s_cliff2, "./assets/cliff2.sgi", FILTER_TRILINEAR, &st) != 0)
			goto return_failure;

		if (TextureInit(&s_grass, "./assets/grass.sgi", FILTER_TRILINEAR, &st) != 0)
			goto return_failure;

		if (TextureInit(&s_dirt, "./assets/dirt.sgi", FILTER_TRILINEAR, &st) != 0)
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
		struct Matrix4 matrix = {0};

		int draw_calls = 0;
		char title[128] = {0};

		bool a_release = false;
		bool b_release = false;
		bool x_release = false;
		bool y_release = false;

		Play2d(s_mixer, 0.7f, PLAY_LOOP, "./assets/ambient01.au");
		SetProgram(s_context, &s_terrain_program);
		SetTexture(s_context, 0, &s_terrain_normalmap);
		SetTexture(s_context, 1, &s_cliff1);
		SetTexture(s_context, 2, &s_cliff2);
		SetTexture(s_context, 3, &s_grass);
		SetTexture(s_context, 4, &s_dirt);

		if (MixerStart(s_mixer, &st) != 0)
			goto return_failure;

		s_terrain_view.aspect = (float)WINDOWS_WIDTH / (float)WINDOWS_HEIGHT;
		s_terrain_view.max_distance = 1024.0f;
		s_terrain_view.fov = DegToRad(FOV);

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

				s_terrain_view.angle = camera->co.angle;
				s_terrain_view.position = camera->co.position;
			}

			if (events.resized == true)
			{
				sSetProjection(events.window_size, s_context);
				s_terrain_view.aspect = (float)events.window_size.x / (float)events.window_size.y;
			}

			// Render terrain
			draw_calls = NTerrainDraw(s_context, s_terrain, &s_terrain_view);

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
	OptionsDelete(s_options);
	DictionaryDelete(s_classes);
	ListClean(&s_entities);

	ProgramFree(&s_terrain_program);
	TextureFree(&s_terrain_normalmap);
	TextureFree(&s_cliff1);
	TextureFree(&s_cliff2);
	TextureFree(&s_grass);
	TextureFree(&s_dirt);
	NTerrainDelete(s_terrain);

	MixerDelete(s_mixer);
	ContextDelete(s_context);

	return EXIT_SUCCESS;

return_failure:
	// TODO, clean... :(
	StatusPrint(NAME_SHORT, st);
	return EXIT_FAILURE;
}
