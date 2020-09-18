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
	unsigned time_vm_med;
	unsigned time_vm_max;
	unsigned time_render_med;
	unsigned time_render_max;

	float showcase_angle;

	mrb_state* state;
	mrb_value program;

	mrb_sym init_symbol;
	mrb_sym frame_symbol;
	mrb_sym resize_symbol;
	mrb_sym function_symbol;
	mrb_sym close_symbol;
};


mrb_value VmCallPrint(mrb_state* state, mrb_value self)
{
	// TODO, add the license:
	// https://github.com/mruby/mruby/blob/master/mrbgems/mruby-print/src/print.c

	(void)self;
	mrb_value argv;

	mrb_get_args(state, "o", &argv);

	if (mrb_string_p(argv)) // Same as 'mrb_type(o) == MRB_TT_STRING'
	{
		fwrite(RSTRING_PTR(argv), (size_t)RSTRING_LEN(argv), 1, stdout);
		fflush(stdout);
	}

	return argv;
}


mrb_value VmCallRequire(mrb_state* state, mrb_value self)
{
	(void)self;
	mrb_value filename;

	mrb_get_args(state, "S", &filename);

	if (mrb_type(filename) != MRB_TT_STRING)
	{
		printf("Error!, the argument isn't an string?\n");
		return mrb_fixnum_value(1);
	}

	printf("Required file '%s', blah, blah, blah...\n", mrb_string_cstr(state, filename));
	return mrb_fixnum_value(0);
}


static inline void sCheckException(struct mrb_state* state)
{
	if (state->exc != NULL)
	{
		printf("[MRuby error]\n");
		mrb_print_error(state);

		state->exc = NULL;
		exit(1);
	}
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
	data->function_symbol = mrb_intern_cstr(data->state, "NaraFunction");
	data->close_symbol = mrb_intern_cstr(data->state, "NaraClose");

	// Define basic functions
	mrb_define_method(data->state, data->state->kernel_module, "print", VmCallPrint, MRB_ARGS_REQ(1));
	mrb_define_method(data->state, data->state->kernel_module, "require", VmCallRequire, MRB_ARGS_REQ(1));

	// Load the program
	data->program = mrb_load_file(data->state, fp);
	sCheckException(data->state);

	fclose(fp);

	// First game callback
	mrb_funcall_argv(data->state, data->program, data->init_symbol, 0, NULL); // 0 arguments, with NULL as them
	sCheckException(data->state);
}


static void sFrame(struct kaWindow* w, struct kaEvents e, float delta, void* raw_data, struct jaStatus* st)
{
	(void)e;
	(void)st;

	struct NaraData* data = raw_data;
	unsigned start = 0;
	unsigned diff = 0;

	// Game frame callback
	start = kaGetTime();
	{
		struct mrb_value delta_vmized = mrb_float_value(data->state, delta);
		mrb_funcall_argv(data->state, data->program, data->frame_symbol, 1, &delta_vmized);
		sCheckException(data->state);
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
	(void)st;
	struct NaraData* data = raw_data;

	// Game resize callback
	struct mrb_value args_vmized[2] = {mrb_fixnum_value(width), mrb_fixnum_value(height)};
	mrb_funcall_argv(data->state, data->program, data->resize_symbol, 2, args_vmized);
	sCheckException(data->state);

	// Render
	float aspect = (float)width / (float)height;

	kaSetWorld(w, jaMatrix4Perspective(jaDegToRad(45.0f), aspect, 0.1f, 500.0f));
	kaSetCameraLookAt(w, (struct jaVector3){0.0f, 0.0f, 0.0f}, (struct jaVector3){2.0f, 2.0f, 2.0f});
}


static void sFunctionKey(struct kaWindow* w, int f, void* raw_data, struct jaStatus* st)
{
	(void)w;
	(void)st;
	struct NaraData* data = raw_data;

	// Game function callback
	struct mrb_value f_vmized = mrb_fixnum_value(f);
	mrb_funcall_argv(data->state, data->program, data->function_symbol, 1, &f_vmized);
	sCheckException(data->state);

	// Render
	if (f == 11)
		kaSwitchFullscreen(w);
}


static void sClose(struct kaWindow* w, void* raw_data)
{
	(void)w;
	struct NaraData* data = raw_data;

	// Game quit callback
	mrb_funcall_argv(data->state, data->program, data->close_symbol, 0, NULL); // 0 arguments, with NULL as them
	sCheckException(data->state);

	// Bye!
	mrb_close(data->state);
	printf("Times, vm: %u ms (%u ms max), render: %u ms (%u ms max)\n", data->time_vm_med, data->time_vm_max,
	       data->time_render_med, data->time_render_max);
}


int main()
{
	struct jaStatus st = {0};
	struct NaraData* data = NULL;

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
