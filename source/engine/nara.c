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

#include "mruby.h"
#include "mruby/array.h"
#include "mruby/compile.h"
#include "mruby/dump.h"
#include "mruby/error.h"
#include "mruby/string.h"
#include "mruby/variable.h"
#include "mruby/version.h"

#define NAME "Nara"
#define VERSION "0.4-alpha"
#define CAPTION "Nara | v0.4-alpha"


struct NaraData
{
	float showcase_angle;

	mrb_state* state;
	mrb_value program;

	mrb_sym init_symbol;
	mrb_sym frame_symbol;
	mrb_sym resize_symbol;
	mrb_sym close_symbol;
};


mrb_value VmCallPrint(mrb_state* state, mrb_value self)
{
	// TODO, add the license:
	// https://github.com/mruby/mruby/blob/master/mrbgems/mruby-print/src/print.c

	(void)self;
	mrb_value argv;

	mrb_get_args(state, "o", &argv);

	if (mrb_string_p(argv))
	{
		fwrite(RSTRING_PTR(argv), (size_t)RSTRING_LEN(argv), 1, stdout);
		fflush(stdout);
	}

	return argv;
}


static void sInit(struct kaWindow* w, void* raw_data, struct jaStatus* st)
{
	(void)w;
	struct NaraData* data = raw_data;

	// Initialize state
	FILE* fp = NULL;

	if ((fp = fopen("./source/game/nara.rb", "r")) == NULL)
	{
		jaStatusSet(st, "sInit", JA_STATUS_FS_ERROR, "'%s'", "./source/game/nara.rb");
		return;
	}

	if ((data->state = mrb_open_core(mrb_default_allocf, NULL)) == NULL)
	{
		jaStatusSet(st, "sInit", JA_STATUS_ERROR, "mrb_open_core()");
		return;
	}

	// Create frequently used symbols (being uint32_t they are cheaper than strings)
	data->init_symbol = mrb_intern_cstr(data->state, "NaraInit");
	data->frame_symbol = mrb_intern_cstr(data->state, "NaraFrame");
	data->resize_symbol = mrb_intern_cstr(data->state, "NaraResize");
	data->close_symbol = mrb_intern_cstr(data->state, "NaraClose");

	// Define basic functions
	mrb_define_method(data->state, data->state->kernel_module, "print", VmCallPrint, MRB_ARGS_REQ(1));

	// Load the program
	data->program = mrb_load_file(data->state, fp);
	fclose(fp);

	// First call
	mrb_funcall_argv(data->state, data->program, data->init_symbol, 0, NULL); // 0 arguments, with NULL as them
}


static void sFrame(struct kaWindow* w, struct kaEvents e, float delta, void* raw_data, struct jaStatus* st)
{
	(void)e;
	(void)st;

	struct NaraData* data = raw_data;

	data->showcase_angle += jaDegToRad(1.0f) * delta;
	kaSetLocal(w, jaMatrix4RotateZ(jaMatrix4Identity(), data->showcase_angle));

	kaDrawDefault(w);
}


static void sResize(struct kaWindow* w, int width, int height, void* raw_data, struct jaStatus* st)
{
	(void)st;
	struct NaraData* data = raw_data;

	float aspect = (float)width / (float)height;

	kaSetWorld(w, jaMatrix4Perspective(jaDegToRad(45.0f), aspect, 0.1f, 500.0f));
	kaSetCameraLookAt(w, (struct jaVector3){0.0f, 0.0f, 0.0f}, (struct jaVector3){2.0f, 2.0f, 2.0f});

	mrb_funcall_argv(data->state, data->program, data->resize_symbol, 0, NULL); // 0 arguments, with NULL as them
}


static void sFunctionKey(struct kaWindow* w, int f, void* raw_data, struct jaStatus* st)
{
	(void)w;
	(void)raw_data;
	(void)st;

	if (f == 11)
		kaSwitchFullscreen(w);
}


static void sClose(struct kaWindow* w, void* raw_data)
{
	(void)w;
	struct NaraData* data = raw_data;

	mrb_funcall_argv(data->state, data->program, data->close_symbol, 0, NULL); // 0 arguments, with NULL as them
	mrb_close(data->state);
}


int main()
{
	struct jaStatus st = {0};
	struct NaraData* data = NULL;

	// Game as normal
	printf("%s v%s\n", NAME, VERSION);
	printf(" - LibJapan %i.%i.%i\n", jaVersionMajor(), jaVersionMinor(), jaVersionPatch());
	printf(" - LibKansai %i.%i.%i\n", kaVersionMajor(), kaVersionMinor(), kaVersionPatch());
	printf(" - mruby %i.%i.%i\n", MRUBY_RELEASE_MAJOR, MRUBY_RELEASE_MINOR, MRUBY_RELEASE_TEENY);

	if ((data = calloc(1, sizeof(struct NaraData))) == NULL)
	{
		jaStatusSet(&st, "main", JA_STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	if (kaContextStart(&st) != 0)
		goto return_failure;

	if (kaWindowCreate(CAPTION, sInit, sFrame, sResize, sFunctionKey, sClose, data, &st) != 0)
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
	return EXIT_SUCCESS;

return_failure:
	jaStatusPrint(NAME, st);
	kaContextStop();
	return EXIT_FAILURE;
}
