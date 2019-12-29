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

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "japan-utilities.h"

#include "../engine/context.h"
#include "../engine/entity.h"
#include "../engine/mixer.h"
#include "../engine/nterrain.h"
#include "../engine/timer.h"

#include "game.h"

#define NAME "Nara v0.2-alpha"
#define NAME_SHORT "Nara"

#define FOV 75.0f

extern const uint8_t g_terrain_vertex[];
extern const uint8_t g_terrain_fragment[];

static struct Context* s_context = NULL;
static struct Mixer* s_mixer = NULL;
static struct Timer s_timer = {0};

static struct NTerrain* s_terrain = NULL;
static struct NTerrainView s_terrain_view;
static struct Program s_terrain_program = {0};
static struct Texture s_terrain_normalmap = {0};

static struct Texture s_cliff1 = {0};
static struct Texture s_cliff2 = {0};
static struct Texture s_grass = {0};
static struct Texture s_dirt = {0};

static struct jaDictionary* s_classes = NULL;
static struct jaList s_entities = {0};

static struct jaConfig* s_config = NULL;


/*-----------------------------

 sInitializeConfiguration()
-----------------------------*/
static struct jaConfig* sInitializeConfiguration(int argc, const char* argv[])
{
	// TODO, check errors
	struct jaConfig* config = jaConfigCreate();

	if (config != NULL)
	{
		jaConfigRegister(config, "render.width", 1200, 320, INT_MAX, NULL);
		jaConfigRegister(config, "render.height", 600, 240, INT_MAX, NULL);
		jaConfigRegister(config, "render.fullscreen", 0, 0, 1, NULL);
		jaConfigRegister(config, "render.vsync", 1, 0, 1, NULL);
		jaConfigRegister(config, "render.samples", 2, 0, 16, NULL);
		jaConfigRegister(config, "render.wireframe", 0, 0, 1, NULL);

		jaConfigRegister(config, "render.max_distance", 1024.0f, 100.0f, 4096.0f, NULL);
		jaConfigRegister(config, "render.filter", "trilinear",
		                        "pixel, bilinear, trilinear, pixel_bilinear, pixel_trilinear", NULL, NULL);

		jaConfigRegister(config, "sound.volume", 0.8f, 0.0f, 1.0f, NULL); // TODO: 0.0f didn't disable the mixer
		jaConfigRegister(config, "sound.frequency", 48000, 8000, 48000, NULL); // TODO: low frequencies = white spaces in resamples
		jaConfigRegister(config, "sound.channels", 2, 1, 2, NULL); // Ok, TODO: zero channels = crash

		jaConfigRegister(config, "sound.max_sounds", 32, 0, 64, NULL);
		jaConfigRegister(config, "sound.sampling", "sinc_medium",
		                        "linear, zero_order, sinc_low, sinc_medium, sinc_high", NULL, NULL);

		jaConfigRegister(config, "terrain.subdivisions", 3, 0, 6, NULL);
		jaConfigRegister(config, "terrain.lod_factor", 0, 0, 6, NULL);

		jaConfigRegister(config, "sensitivity", 1.0f, 0.0f, 10.0f, NULL);
		jaConfigRegister(config, "fov", 90.0f, 10.0f, 90.0f, NULL);
	}

	jaConfigReadFile(config, "user.jcfg", NULL);
	jaConfigReadArguments(config, argc, argv, NULL);

	return config;
}


/*-----------------------------

 sInitializeClasses()
-----------------------------*/
static struct jaDictionary* sInitializeClasses()
{
	// TODO, check errors
	struct jaDictionary* classes = jaDictionaryCreate(NULL);
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
static void sSetProjection(struct Context* context)
{
	struct jaVector2i window_size = GetWindowSize(context);
	struct jaMatrix4 projection;

	projection = jaMatrix4Perspective(jaDegToRad(FOV), (float)window_size.x / (float)window_size.y, 0.1f, 1024.0f);
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
static void sFullscreen(const struct Context* context, const struct ContextEvents* events, bool press)
{
	(void)context;
	(void)events;

	if (press == true)
		printf("Hai, hai...\n");
}

static void sScreenshot(const struct Context* context, const struct ContextEvents* events, bool press)
{
	(void)context;
	(void)events;

	if (press == true)
	{
		char timestr[64] = {0};
		char filename[64] = {0};

		time_t t;
		time(&t);

		strftime(timestr, 64, "%Y-%m-%e, %H:%M:%S", localtime(&t));
		sprintf(filename, "%s - %s (%lX).sgi", NAME_SHORT, timestr, s_timer.frame_number);
		TakeScreenshot(context, filename, NULL);
	}
}

int main(int argc, const char* argv[])
{
	struct jaStatus st = {0};
	struct Entity* camera = NULL;

	printf("%s\n", NAME);

	if ((s_config = sInitializeConfiguration(argc, argv)) == NULL)
	{
		jaStatusSet(&st, "sInitializeConfiguration", STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	// Engine initialization
	if ((s_context = ContextCreate(s_config, NAME, &st)) == NULL)
		goto return_failure;

	if ((s_mixer = MixerCreate(s_config, &st)) == NULL)
		goto return_failure;

	TimerInit(&s_timer);

	SetFunctionKeyCallback(s_context, 11, sFullscreen);
	SetFunctionKeyCallback(s_context, 12, sScreenshot);

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
		camera->co.position = (struct jaVector3){700.0f, 700.0f, 256.0f};
		camera->co.angle = (struct jaVector3){-50.0f, 0.0f, 45.0f};

		sSetProjection(s_context);
	}

	// Game loop
	{
		struct ContextEvents events = {0};
		struct EntityInput entities_input = {0};
		struct jaMatrix4 matrix;

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

		s_terrain_view.aspect = (float)GetWindowSize(s_context).x / (float)GetWindowSize(s_context).y;
		s_terrain_view.max_distance = 1024.0f;
		s_terrain_view.fov = jaDegToRad(FOV);

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
			if (jaVector3Equals(camera->co.position, camera->old_co.position) == false ||
			    jaVector3Equals(camera->co.angle, camera->old_co.angle) == false)
			{
				matrix = jaMatrix4Identity();
				matrix = jaMatrix4RotateX(matrix, jaDegToRad(camera->co.angle.x));
				matrix = jaMatrix4RotateZ(matrix, jaDegToRad(camera->co.angle.z));
				matrix = jaMatrix4Multiply(matrix, jaMatrix4Translate(jaVector3Invert(camera->co.position)));

				SetCamera(s_context, matrix, camera->co.position);

				s_terrain_view.angle = camera->co.angle;
				s_terrain_view.position = camera->co.position;
			}

			if (events.resized == true)
			{
				sSetProjection(s_context);
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
	jaConfigDelete(s_config);
	jaDictionaryDelete(s_classes);
	jaListClean(&s_entities);

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
	jaStatusPrint(NAME_SHORT, st);
	return EXIT_FAILURE;
}
