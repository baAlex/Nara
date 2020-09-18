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

 [vm.c]
 - Alexander Brandt 2020
-----------------------------*/

#include "vm.h"

#include <stdlib.h>

#include "mruby.h"
#include "mruby/array.h"
#include "mruby/compile.h"
#include "mruby/dump.h"
#include "mruby/error.h"
#include "mruby/string.h"
#include "mruby/variable.h"

struct Vm
{
	mrb_state* state;
	mrb_value program;

	mrb_sym init_symbol;
	mrb_sym frame_symbol;
	mrb_sym resize_symbol;
	mrb_sym function_symbol;
	mrb_sym close_symbol;
};


static mrb_value sVmCallRequire(mrb_state* state, mrb_value self)
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


static inline int sCheckException(struct mrb_state* state, struct jaStatus* st)
{
	if (state->exc != NULL)
	{
		printf("\n");
		mrb_print_error(state);

		state->exc = NULL;

		jaStatusSet(st, "mruby", JA_STATUS_ERROR, NULL);
		return 1;
	}

	return 0;
}


struct Vm* VmCreate(const char* main_filename, struct jaStatus* st)
{
	struct Vm* vm = NULL;
	FILE* fp = NULL;

	jaStatusSet(st, "VmCreate", JA_STATUS_SUCCESS, NULL);

	if ((vm = calloc(1, sizeof(struct Vm))) == NULL)
		goto return_failure;

	// Initialize state
	if ((fp = fopen(main_filename, "r")) == NULL)
	{
		jaStatusSet(st, "sInit", JA_STATUS_FS_ERROR, "'%s'", main_filename);
		goto return_failure;
	}

	if ((vm->state = mrb_open()) == NULL)
	{
		jaStatusSet(st, "sInit", JA_STATUS_ERROR, "mrb_open()");
		goto return_failure;
	}

	// Create frequently used symbols (being uint32_t they are cheaper than strings)
	vm->init_symbol = mrb_intern_cstr(vm->state, "NaraInit");
	vm->frame_symbol = mrb_intern_cstr(vm->state, "NaraFrame");
	vm->resize_symbol = mrb_intern_cstr(vm->state, "NaraResize");
	vm->function_symbol = mrb_intern_cstr(vm->state, "NaraFunction");
	vm->close_symbol = mrb_intern_cstr(vm->state, "NaraClose");

	// Define basic functions
	mrb_define_method(vm->state, vm->state->kernel_module, "require", sVmCallRequire, MRB_ARGS_REQ(1));

	// Load the program
	vm->program = mrb_load_file(vm->state, fp);

	if (sCheckException(vm->state, st) != 0)
		goto return_failure;

	// Bye!
	fclose(fp);
	return vm;

return_failure:
	if (fp != NULL)
		fclose(fp);
	if (vm != NULL)
		VmDelete(vm);
	return NULL;
}


void VmDelete(struct Vm* vm)
{
	mrb_close(vm->state);
}


int VmCallbackInit(struct Vm* vm, struct jaStatus* st)
{
	mrb_funcall_argv(vm->state, vm->program, vm->init_symbol, 0, NULL); // 0 arguments, with NULL as them
	return sCheckException(vm->state, st);
}


int VmCallbackFrame(struct Vm* vm, float delta, struct jaStatus* st)
{
	struct mrb_value delta_vmized = mrb_float_value(vm->state, delta);

	mrb_funcall_argv(vm->state, vm->program, vm->frame_symbol, 1, &delta_vmized);
	return sCheckException(vm->state, st);
}


int VmCallbackResize(struct Vm* vm, int width, int height, struct jaStatus* st)
{
	struct mrb_value args_vmized[2] = {mrb_fixnum_value(width), mrb_fixnum_value(height)};

	mrb_funcall_argv(vm->state, vm->program, vm->resize_symbol, 2, args_vmized);
	return sCheckException(vm->state, st);
}


int VmCallbackFunction(struct Vm* vm, int f, struct jaStatus* st)
{
	struct mrb_value f_vmized = mrb_fixnum_value(f);

	mrb_funcall_argv(vm->state, vm->program, vm->function_symbol, 1, &f_vmized);
	return sCheckException(vm->state, st);
}


int VmCallbackQuit(struct Vm* vm)
{
	mrb_funcall_argv(vm->state, vm->program, vm->close_symbol, 0, NULL); // 0 arguments, with NULL as them
	return sCheckException(vm->state, NULL);
}
