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
 - Alexander Brandt 2019-2020
-----------------------------*/

#include <stdio.h>

#include "japan-configuration.h"
#include "japan-dictionary.h"
#include "japan-status.h"

#include "context/context.h"
#include "entity.h"
#include "mixer/mixer.h"
#include "nterrain.h"
#include "timer.h"

#include "../game/game.h"

#define NAME "Nara v0.4-alpha"
#define NAME_SHORT "Nara"
#define NAME_SIMPLE "nara"

extern const uint8_t g_terrain_vertex[];
extern const uint8_t g_terrain_fragment[];
extern const uint8_t g_debug_vertex[];
extern const uint8_t g_debug_fragment[];

struct
{
	struct jaConfiguration* config;
	struct Context* context;
	struct Mixer* mixer;
	struct Timer timer;
	struct jaDictionary* classes;

} modules;

struct Resources
{
	struct NTerrain* terrain;
	struct Program terrain_program;

	struct Program debug_program;

	struct Texture lightmap;
	struct Texture masksmap;
	struct Texture grass;
	struct Texture dirt;
	struct Texture cliff1;
	struct Texture cliff2;

	struct jaList entities;
	struct Entity* camera;

} resources;


/*-----------------------------

 sInitializeConfiguration()
-----------------------------*/
static struct jaConfiguration* sInitializeConfiguration(int argc, const char* argv[], struct jaStatus* st)
{
	struct jaConfiguration* config = jaConfigurationCreate();

	if (config == NULL)
		goto return_failure;

	if (jaCvarCreate(config, "render.width", 1200, 320, INT_MAX, st) == NULL ||
	    jaCvarCreate(config, "render.height", 600, 240, INT_MAX, st) == NULL ||
	    jaCvarCreate(config, "render.fullscreen", 0, 0, 1, st) == NULL ||
	    jaCvarCreate(config, "render.vsync", 1, 0, 1, st) == NULL ||
	    jaCvarCreate(config, "render.samples", 1, 0, 16, st) == NULL ||
	    jaCvarCreate(config, "render.wireframe", 0, 0, 1, st) == NULL ||
	    jaCvarCreate(config, "render.filter", "trilinear",
	                 "pixel, bilinear, trilinear, pixel_bilinear, pixel_trilinear", NULL, st) == NULL)
		goto return_failure;

	if (jaCvarCreate(config, "mixer.volume", 0.8f, 0.0f, 2.0f, st) == NULL ||
	    jaCvarCreate(config, "mixer.frequency", 48000, 8000, 48000, st) == NULL ||
	    jaCvarCreate(config, "mixer.channels", 2, 1, 2, st) == NULL ||
	    jaCvarCreate(config, "mixer.max_sounds", 32, 0, 64, st) == NULL ||
	    jaCvarCreate(config, "mixer.sampling", "linear", "linear, zero_order, sinc_low, sinc_medium, sinc_high", NULL,
	                 st) == NULL)
		goto return_failure;

	if (jaCvarCreate(config, "limiter.threshold", 0.7f, 0.0f, 1.0f, st) == NULL ||
	    jaCvarCreate(config, "limiter.attack", 0.0f, 0.0f, 666.f, st) == NULL || // TODO
	    jaCvarCreate(config, "limiter.release", 0.0f, 0.0, 666.f, st) == NULL)   // TODO
		goto return_failure;

	if (jaCvarCreate(config, "lod.terrain", 1, 1, 4, st) == NULL)
		goto return_failure;

	if (jaCvarCreate(config, "max_distance", 1024.0f, 128.0f, 1024.f, st) == NULL ||
	    jaCvarCreate(config, "fov", 75.0f, 45.0f, 90.0f, st) == NULL)
		goto return_failure;

	jaConfigurationFile(config, "user.jcfg", NULL); // TODO
	jaConfigurationArguments(config, argc, argv);   // "

	// Bye!
	return config;

return_failure:
	if (config != NULL)
		jaConfigurationDelete(config);
	return NULL;
}


/*-----------------------------

 sInitializeClasses()
-----------------------------*/
static struct jaDictionary* sInitializeClasses(struct jaStatus* st)
{
	struct jaDictionary* classes = jaDictionaryCreate(NULL);
	struct Class* class = NULL;

	if (classes == NULL)
		goto return_failure;

	if ((class = ClassCreate(classes, "Camera")) == NULL)
		goto return_failure;

	class->func_start = GameCameraStart;
	class->func_think = GameCameraThink;
	class->func_delete = GameCameraDelete;

	if ((class = ClassCreate(classes, "Point")) == NULL)
		goto return_failure;

	class->func_start = GamePointStart;
	class->func_think = GamePointThink;
	class->func_delete = GamePointDelete;

	// Bye!
	return classes;

return_failure:
	jaStatusSet(st, "sInitializeClasses", STATUS_MEMORY_ERROR, NULL);

	if (classes != NULL)
		jaDictionaryDelete(classes);
	return NULL;
}


/*-----------------------------

 sStatsCallback()
-----------------------------*/
static void sStatsCallback(const struct Context* context, const struct ContextEvents* events, bool press, void* data)
{
	(void)events;
	struct Timer* timer = data;
	struct jaVector3 cam = GetCameraOrigin(context);

	if (press == true)
	{
		printf("Fps: %i (~%.02f), DCalls: %i\n", timer->frames_per_second, timer->miliseconds_betwen,
		       GetDrawCalls(context));
		printf("Camera at: {.x = %0.2ff, .y = %0.2ff, .z = %0.2ff}\n", cam.x, cam.y, cam.z);
	}
}


/*-----------------------------

 sScreenshotCallback()
-----------------------------*/
static void sScreenshotCallback(const struct Context* context, const struct ContextEvents* events, bool press,
                                void* data)
{
	(void)context;
	(void)events;
	struct Timer* timer = data;

	if (press == true)
	{
		char tstr[64];
		char filename[64];

		time_t t;
		time(&t);

		strftime(tstr, 64, "%Y-%m-%e, %H:%M:%S", localtime(&t));
		sprintf(filename, "%s - %s (%lX).sgi", NAME_SIMPLE, tstr, timer->frame_number);
		TakeScreenshot(context, filename, NULL);
	}
}


/*-----------------------------

 sUnloadResources()
-----------------------------*/
static inline void sUnloadResources(struct Resources* res)
{
	NTerrainDelete(res->terrain);
	ProgramFree(&res->terrain_program);
	ProgramFree(&res->debug_program);
	TextureFree(&res->lightmap);
	TextureFree(&res->masksmap);
	TextureFree(&res->grass);
	TextureFree(&res->dirt);
	TextureFree(&res->cliff1);
	TextureFree(&res->cliff2);
}


/*-----------------------------

 sLoadResources()
-----------------------------*/
static int sLoadResources(struct Context* context, struct Mixer* mixer, struct jaDictionary* classes,
                          struct Resources* out, struct jaStatus* st)
{
	jaStatusSet(st, "sLoadResources", STATUS_SUCCESS, NULL);

	if ((out->terrain = NTerrainCreate("./assets/heightmap.sgi", 150.0, 1944.0, 24.0, 2, st)) == NULL)
		goto return_failure;

	if (ProgramInit((char*)g_terrain_vertex, (char*)g_terrain_fragment, &out->terrain_program, st) != 0)
		goto return_failure;

	if (ProgramInit((char*)g_debug_vertex, (char*)g_debug_fragment, &out->debug_program, st) != 0)
		goto return_failure;

	if (TextureInit(context, "./assets/lightmap.sgi", &out->lightmap, st) != 0 ||
	    TextureInit(context, "./assets/masksmap.sgi", &out->masksmap, st) != 0 ||
	    TextureInit(context, "./assets/grass.sgi", &out->grass, st) != 0 ||
	    TextureInit(context, "./assets/dirt.sgi", &out->dirt, st) != 0 ||
	    TextureInit(context, "./assets/cliff1.sgi", &out->cliff1, st) != 0 ||
	    TextureInit(context, "./assets/cliff2.sgi", &out->cliff2, st) != 0)
		goto return_failure;

	if (SampleCreate(mixer, "./assets/ambient01.au", st) == NULL || // Works as precache method
	    SampleCreate(mixer, "./assets/silly-loop.au", st) == NULL)  // "
		goto return_failure;

	// Set them
	SetTexture(context, 0, &out->lightmap);
	SetTexture(context, 1, &out->masksmap);
	SetTexture(context, 2, &out->grass);
	SetTexture(context, 3, &out->dirt);
	SetTexture(context, 4, &out->cliff1);
	SetTexture(context, 5, &out->cliff2);

	// Entities (just two)
	jaListClean(&out->entities);

	EntityCreate(&out->entities, ClassGet(classes, "Point"));
	out->camera = EntityCreate(&out->entities, ClassGet(classes, "Camera"));
	out->camera->co.position = (struct jaVector3){700.0f, 700.0f, 256.0f};
	out->camera->co.angle = (struct jaVector3){-50.0f, 0.0f, 45.0f};

	// Bye!
	NTerrainPrintInfo(out->terrain);
	return 0;

return_failure:
	sUnloadResources(out);
	return 1;
}


/*-----------------------------

 sSetProjectionAndView()
-----------------------------*/
static void sSetProjectionAndView(const struct jaCvar* fov_cvar, const struct jaCvar* max_distance_cvar,
                                  const struct jaCvar* lod_terrain_cvar, struct Context* out_context,
                                  struct NTerrainView* out_view)
{
	struct jaMatrix4 projection_matrix;

	float aspect = (float)GetWindowSize(out_context).x / (float)GetWindowSize(out_context).y;
	float fov = 90.0f;
	float max_distance = 1024.0f;

	jaCvarValue(fov_cvar, &fov, NULL);
	jaCvarValue(max_distance_cvar, &max_distance, NULL);

	projection_matrix = jaMatrix4Perspective(jaDegToRad(fov), aspect, 0.1f, max_distance);
	SetProjection(out_context, projection_matrix);

	out_view->fov = jaDegToRad(fov);
	out_view->aspect = aspect;
	out_view->max_distance = max_distance;

	jaCvarValue(lod_terrain_cvar, &out_view->lod_factor, NULL);

	if (out_view->lod_factor > 1) // In steps of 3
		out_view->lod_factor = (3 * out_view->lod_factor) - 3;
}


/*-----------------------------

 sCreateEntitiesInput()
-----------------------------*/
static struct EntityInput sCreateEntitiesInput(struct ContextEvents* events, double ms_betwen)
{
	struct EntityInput in = {0};

	in.a = events->a;
	in.b = events->b;
	in.x = events->x;
	in.y = events->y;
	in.lb = events->lb;
	in.rb = events->rb;
	in.view = events->view;
	in.menu = events->menu;
	in.guide = events->guide;
	in.ls = events->ls;
	in.rs = events->rs;
	in.pad.h = events->pad.h;
	in.pad.v = events->pad.v;
	in.left_analog.h = events->left_analog.h;
	in.left_analog.v = events->left_analog.v;
	in.left_analog.t = events->left_analog.t;
	in.right_analog.h = events->right_analog.h;
	in.right_analog.v = events->right_analog.v;
	in.right_analog.t = events->right_analog.t;

	in.delta = (float)ms_betwen / 33.3333f;

	return in;
}


/*-----------------------------

 main()
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

int main(int argc, const char* argv[])
{
	struct jaStatus st = {0};

	struct jaCvar* max_distance;
	struct jaCvar* fov;
	struct jaCvar* lod_terrain;

	printf("%s\n\n", NAME);

	// Initialization
	if ((modules.config = sInitializeConfiguration(argc, argv, &st)) == NULL)
		goto return_failure;

	if ((modules.classes = sInitializeClasses(&st)) == NULL)
		goto return_failure;

	if ((modules.context = ContextCreate(modules.config, NAME, &st)) == NULL)
		goto return_failure;

	if ((modules.mixer = MixerCreate(modules.config, &st)) == NULL)
		goto return_failure;

	TimerInit(&modules.timer);

	SetFunctionKeyCallback(modules.context, 9, &modules.timer, sStatsCallback);
	SetFunctionKeyCallback(modules.context, 12, &modules.timer, sScreenshotCallback);

	if (sLoadResources(modules.context, modules.mixer, modules.classes, &resources, &st) != 0)
		goto return_failure;

	// Game loop
	struct ContextEvents events = {0};
	struct NTerrainView view;
	struct jaMatrix4 matrix_camera;

	char temp_str[64];

	bool release_a = false;
	bool release_b = false;
	bool release_x = false;
	bool release_y = false;

	PlayFile(modules.mixer, PLAY_LOOP, 0.5f, "./assets/ambient01.au");
	Play3dFile(modules.mixer, PLAY_LOOP, 1.0f, (struct PlayRange){.min = 50.0f, .max = 700.0f},
	           (struct jaVector3){.x = 1166.38f, .y = 1660.34f, .z = 102.28f}, "./assets/silly-loop.au");

	max_distance = jaCvarGet(modules.config, "max_distance");
	fov = jaCvarGet(modules.config, "fov");
	lod_terrain = jaCvarGet(modules.config, "lod.terrain");
	sSetProjectionAndView(fov, max_distance, lod_terrain, modules.context, &view);

	while (1)
	{
		// Update engine
		TimerUpdate(&modules.timer);
		ContextUpdate(modules.context, &events);

		EntitiesUpdate(&resources.entities, sCreateEntitiesInput(&events, modules.timer.miliseconds_betwen));

		if (jaVector3Equals(resources.camera->co.position, resources.camera->old_co.position) == false ||
		    jaVector3Equals(resources.camera->co.angle, resources.camera->old_co.angle) == false)
		{
			matrix_camera = jaMatrix4Identity();
			matrix_camera = jaMatrix4RotateX(matrix_camera, jaDegToRad(resources.camera->co.angle.x));
			matrix_camera = jaMatrix4RotateZ(matrix_camera, jaDegToRad(resources.camera->co.angle.z));
			matrix_camera =
			    jaMatrix4Multiply(matrix_camera, jaMatrix4Translate(jaVector3Invert(resources.camera->co.position)));

			SetCamera(modules.context, matrix_camera, resources.camera->co.position);
			SetListener(modules.mixer, resources.camera->co.position, resources.camera->co.angle);

			view.angle = resources.camera->co.angle;
			view.position = resources.camera->co.position;
		}

		if (events.resized == true)
			sSetProjectionAndView(fov, max_distance, lod_terrain, modules.context, &view);

		// Render
		SetProgram(modules.context, &resources.terrain_program);
		NTerrainDraw(modules.context, resources.terrain, &view);

		SetProgram(modules.context, &resources.debug_program);
		DrawAABB(modules.context, (struct jaAABBox){0}, (struct jaVector3){0.0f, 0.0f, 0.0f});

		// Window title
		if (modules.timer.one_second == true)
		{
			sprintf(temp_str, "%s | fps: %i (~%.02f), dcalls: %i", NAME, modules.timer.frames_per_second,
			        modules.timer.miliseconds_betwen, GetDrawCalls(modules.context));
			SetTitle(modules.context, temp_str);
		}

		// Testing, testing
		if (sSingleClick(events.a, &release_a) == true)
			PlayFile(modules.mixer, PLAY_NORMAL, 1.0f, "./assets/rz1-kick.wav");

		if (sSingleClick(events.b, &release_b) == true)
			PlayFile(modules.mixer, PLAY_NORMAL, 1.0f, "./assets/rz1-snare.wav");

		if (sSingleClick(events.x, &release_x) == true)
			PlayFile(modules.mixer, PLAY_NORMAL, 1.0f, "./assets/rz1-closed-hithat.wav");

		if (sSingleClick(events.y, &release_y) == true)
			PlayFile(modules.mixer, PLAY_NORMAL, 1.0f, "./assets/rz1-clap.wav");

		// Exit?
		if (events.close == true)
			break;
	}

	// Bye!
	sUnloadResources(&resources);
	MixerDelete(modules.mixer);
	ContextDelete(modules.context);
	jaDictionaryDelete(modules.classes);
	jaConfigurationDelete(modules.config);
	return 0;

return_failure:
	jaStatusPrint(NAME_SHORT, st);

	sUnloadResources(&resources);
	MixerDelete(modules.mixer);
	ContextDelete(modules.context);

	if (modules.classes != NULL)
		jaDictionaryDelete(modules.classes);
	if (modules.config != NULL)
		jaConfigurationDelete(modules.config);

	return 1;
}
