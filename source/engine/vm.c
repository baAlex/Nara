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

 [vm.c]
 - Alexander Brandt 2019-2020
-----------------------------*/

#include "vm.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "japan-dictionary.h"

#include <mruby.h>
#include <mruby/array.h>
#include <mruby/compile.h>
#include <mruby/error.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <mruby/version.h>

struct EntityClass
{
	int placeholder;
};

struct Vm
{
	mrb_state* state;
	struct jaDictionary* classes;
};


static mrb_value sVmCallPrint(mrb_state* state, mrb_value self)
{
	// TODO, add the license:
	// https://github.com/mruby/mruby/blob/master/mrbgems/mruby-print/src/print.c

	(void)self;
	mrb_value argv;

	mrb_get_args(state, "o", &argv);

	if (mrb_string_p(argv))
	{
		fwrite(RSTRING_PTR(argv), RSTRING_LEN(argv), 1, stdout);
		fflush(stdout);
	}

	return argv;
}


static inline bool sException(struct mrb_state* state)
{
	if (state->exc != NULL)
	{
		mrb_print_error(state);

		state->exc = NULL;
		return true;
	}

	return false;
}


static inline bool sCheckVariableIn(struct mrb_state* state, struct mrb_value object, mrb_sym symbol,
                                    enum mrb_vtype type, mrb_value* out)
{
	mrb_value temp = {.value.p = NULL};

	if (mrb_iv_defined(state, object, symbol) == 0) // 0 == false
		return false;
	else
	{
		temp = mrb_iv_get(state, object, symbol);

		if (out != NULL)
			*out = temp;

		if (mrb_type(temp) != type)
			return false;
	}

	return true;
}


static inline bool sCheckVector3In(struct mrb_state* state, struct mrb_value object, mrb_sym symbol)
{
	mrb_value temp = {.value.p = NULL};
	const char* class_name = mrb_class_name(state, mrb_obj_ptr(object)->c);

	bool valid = true;

	if (sCheckVariableIn(state, object, symbol, MRB_TT_OBJECT, &temp) == false)
	{
		if (temp.value.p == NULL)
			printf("[Error] '%s' didn't include an '%s' definition\n", class_name, mrb_sym2name(state, symbol));
		else
			printf("[Error] Variable '%s' in '%s' is not an object\n", mrb_sym2name(state, symbol), class_name);

		valid = false;
	}
	else
	{
		if (mrb_iv_defined(state, temp, mrb_intern_cstr(state, "@x")) == 0) // 0 == false
		{
			printf("[Error] Variable '@x' not defined in '%s' '%s' object\n", class_name, mrb_sym2name(state, symbol));
			valid = false;
		}

		if (mrb_iv_defined(state, temp, mrb_intern_cstr(state, "@y")) == 0)
		{
			printf("[Error] Variable '@y' not defined in '%s' '%s' object\n", class_name, mrb_sym2name(state, symbol));
			valid = false;
		}

		if (mrb_iv_defined(state, temp, mrb_intern_cstr(state, "@z")) == 0)
		{
			printf("[Error] Variable '@z' not defined in '%s' '%s' object\n", class_name, mrb_sym2name(state, symbol));
			valid = false;
		}
	}

	return valid;
}


static bool sCheckEntityClass(struct mrb_state* state, const char* class_name)
{
	struct RClass* class = NULL;
	struct mrb_value object = {.value.p = NULL};

	bool valid = true;

	if (mrb_class_defined(state, class_name) == 0)
	{
		printf("[Error] Class '%s' not defined\n", class_name);
		return false;
	}

	class = mrb_class_get(state, class_name); // Always after mrb_class_defined()
	object = mrb_obj_new(state, class, 0, NULL);

	if (sException(state) == true)
	{
		printf("[Error] Exception raised at '%s' initialization\n", class_name);
		goto return_failure;
	}

	// Print all variables
	/*{
	    printf("Class '%s' internal variables:\n", class_name);

	    struct mrb_value variables = mrb_obj_instance_variables(state, object);
	    struct RArray* array = mrb_ary_ptr(variables);

	    for (int i = 0; i < RARRAY_LEN(variables); i++)
	    {
	        mrb_value item = mrb_ary_ref(state, variables, i);
	        printf("- '%s'\n", mrb_sym2name(state, mrb_obj_to_sym(state, item)));
	    }

	    if (RARRAY_LEN(variables) == 0)
	        printf(" - none\n");
	}*/

	// Check definitions needed by the engine
	if (sCheckVector3In(state, object, mrb_intern_cstr(state, "@position")) == false)
		valid = false;

	if (sCheckVector3In(state, object, mrb_intern_cstr(state, "@angle")) == false)
		valid = true;

	// Bye!
	mrb_gc_mark_value(state, object);
	return valid;

return_failure:
	if (object.value.p != NULL)
		mrb_gc_mark_value(state, object);

	return false;
}


struct Vm* VmCreate(const char* filename[], struct jaStatus* st)
{
	(void)st; // TODO

	FILE* file = NULL;
	struct Vm* vm = NULL;

	if ((vm = calloc(1, sizeof(struct Vm))) == NULL)
		return NULL;

	if ((vm->state = mrb_open_core(mrb_default_allocf, NULL)) == NULL)
	{
		jaStatusSet(st, "VmCreate", STATUS_ERROR, "mrb_open_core()");
		goto return_failure;
	}

	mrb_define_method(vm->state, vm->state->object_class, "print", sVmCallPrint, MRB_ARGS_REQ(1));

	// Load files
	for (int i = 0;; i++)
	{
		if (filename[i] == NULL)
			break;

		if ((file = fopen(filename[i], "r")) == NULL)
		{
			jaStatusSet(st, "VmCreate", STATUS_IO_ERROR, "%s", filename[i]);
			goto return_failure;
		}

		mrb_load_file(vm->state, file);
		fclose(file);

		if (sException(vm->state) == true)
		{
			printf("[Error] Exception raised at '%s' load procedure\n", filename[i]);
			goto return_failure;
		}
	}

	// Test test test
	sCheckEntityClass(vm->state, "Camera");
	sCheckEntityClass(vm->state, "Point");
	sCheckEntityClass(vm->state, "Player");

	// Bye!
	return vm;

return_failure:
	VmDelete(vm);
	return NULL;
}


void VmDelete(struct Vm* vm)
{
	if (vm == NULL)
		return;

	if (vm->state != NULL)
		mrb_close(vm->state);

	free(vm);
}
