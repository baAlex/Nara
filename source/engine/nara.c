/*-----------------------------

MIT License

Copyright (c) 2020 Alexander Brandt

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
 - Alexander Brandt 2020
-----------------------------*/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "japan-buffer.h"
#include "japan-matrix.h"
#include "japan-utilities.h"
#include "japan-version.h"
#include "kansai-context.h"
#include "kansai-version.h"

#include "mruby/version.h"
#include "vm.h"

#define NAME "Nara"
#define VERSION "0.4-alpha"
#define CAPTION "Nara | v0.4-alpha"


struct NaraData
{
	unsigned time_vm_med;
	unsigned time_vm_max;
	unsigned time_render_med;
	unsigned time_render_max;

	float showcase_angle;

	struct Vm* vm;
};


static void sInit(struct kaWindow* w, void* raw_data, struct jaStatus* st)
{
	(void)w;
	struct NaraData* data = raw_data;

	if ((data->vm = VmCreate("./source/game/nara.rb", st)) == NULL)
		return;

	if (VmCallbackInit(data->vm, st) != 0)
		return;
}


static void sFrame(struct kaWindow* w, struct kaEvents e, float delta, void* raw_data, struct jaStatus* st)
{
	(void)e;

	struct NaraData* data = raw_data;
	unsigned start = 0;
	unsigned diff = 0;

	// Game frame callback
	start = kaGetTime();
	{
		if (VmCallbackFrame(data->vm, delta, st) != 0)
			return;
	}

	diff = (kaGetTime() - start);
	data->time_vm_med = (data->time_vm_med + diff) >> 1;
	data->time_vm_max = (diff > data->time_vm_max) ? diff : data->time_vm_max;

	// Render
	start = kaGetTime();
	{
		data->showcase_angle += jaDegToRad(1.0f) * delta;

		kaSetLocal(w, jaMatrix4RotateZ(jaMatrix4Identity(), data->showcase_angle));
		kaDrawDefault(w);
	}

	diff = (kaGetTime() - start);
	data->time_render_med = (data->time_render_med + diff) >> 1;
	data->time_render_max = (diff > data->time_render_max) ? diff : data->time_render_max;
}


static void sResize(struct kaWindow* w, int width, int height, void* raw_data, struct jaStatus* st)
{
	struct NaraData* data = raw_data;

	if (VmCallbackResize(data->vm, width, height, st) != 0)
		return;

	// Render
	float aspect = (float)width / (float)height;

	kaSetWorld(w, jaMatrix4Perspective(jaDegToRad(45.0f), aspect, 0.1f, 500.0f));
	kaSetCameraLookAt(w, (struct jaVector3){0.0f, 0.0f, 0.0f}, (struct jaVector3){2.0f, 2.0f, 2.0f});
}


static void sFunctionKey(struct kaWindow* w, int f, void* raw_data, struct jaStatus* st)
{
	(void)w;
	struct NaraData* data = raw_data;

	if (VmCallbackFunction(data->vm, f, st) != 0)
		return;

	// Render
	if (f == 11)
		kaSwitchFullscreen(w);
}


static void sClose(struct kaWindow* w, void* raw_data)
{
	(void)w;
	struct NaraData* data = raw_data;

	if (VmCallbackQuit(data->vm) != 0)
		return;

	// Bye!
	VmDelete(data->vm);
	printf("Times, vm: %u ms (%u ms max), render: %u ms (%u ms max)\n", data->time_vm_med, data->time_vm_max,
	       data->time_render_med, data->time_render_max);
}


static void sArgumentsCallback(enum jaStatusCode code, int i, const char* key, const char* value)
{
	printf("[Warning] %s, argument %i ['%s' = '%s']\n", jaStatusCodeMessage(code), i, key, value);
}

int main(int argc, const char* argv[])
{
	struct jaStatus st = {0};
	struct NaraData* data = NULL;
	struct jaConfiguration* cfg = NULL;

	// Configuration
	if ((cfg = jaConfigurationCreate()) == NULL)
	{
		jaStatusSet(&st, "main", JA_STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	if (jaCvarCreateInt(cfg, "render.width", 720, 360, INT_MAX, &st) == NULL ||
	    jaCvarCreateInt(cfg, "render.height", 480, 240, INT_MAX, &st) == NULL ||
	    jaCvarCreateInt(cfg, "render.fullscreen", 0, 0, 1, &st) == NULL ||
	    jaCvarCreateInt(cfg, "render.vsync", 1, 0, 1, &st) == NULL ||
	    jaCvarCreateString(cfg, "kansai.caption", CAPTION, NULL, NULL, &st) == NULL)
		goto return_failure;

	jaConfigurationArgumentsEx(cfg, JA_UTF8, JA_SKIP_FIRST, sArgumentsCallback, argc, argv);

	// Game as normal
	printf("%s v%s\n", NAME, VERSION);
	printf(" - LibJapan %i.%i.%i\n", jaVersionMajor(), jaVersionMinor(), jaVersionPatch());
	printf(" - LibKansai %i.%i.%i\n", kaVersionMajor(), kaVersionMinor(), kaVersionPatch());
	printf(" - mruby-peko %i.%i.%i\n", MRUBY_RELEASE_MAJOR, MRUBY_RELEASE_MINOR, MRUBY_RELEASE_TEENY);

	if ((data = calloc(1, sizeof(struct NaraData))) == NULL)
	{
		jaStatusSet(&st, "main", JA_STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	if (kaContextStart(&st) != 0)
		goto return_failure;

	if (kaWindowCreate(cfg, sInit, sFrame, sResize, sFunctionKey, sClose, data, &st) != 0)
		goto return_failure;

	// Main loop
	while (1)
	{
		if (kaContextUpdate(&st) != 0)
			break;
	}

	if (st.code != JA_STATUS_SUCCESS)
		goto return_failure;

	// Bye!
	kaContextStop();
	jaConfigurationDelete(cfg);
	free(data);
	return EXIT_SUCCESS;

return_failure:
	jaStatusPrint(NAME, st);
	kaContextStop();
	if (cfg != NULL)
		jaConfigurationDelete(cfg);
	if (data != NULL)
		free(data);
	return EXIT_FAILURE;
}
