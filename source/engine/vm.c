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
#include "japan-list.h"

#include <mruby.h>
#include <mruby/array.h>
#include <mruby/compile.h>
#include <mruby/dump.h>
#include <mruby/error.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <mruby/version.h>

struct NEntityClass
{
	struct RClass* rclass;
};

struct NEntity
{
	struct NEntityState state;

	struct mrb_value robject;
	struct mrb_value position_robject;
	struct mrb_value angle_robject;
};

struct Vm
{
	struct jaDictionary* classes;
	struct jaList entities;
	struct VmGlobals last_globals;

	mrb_state* rstate;
	int rgc_state;

	struct RClass* nara_module;
	struct RClass* input_module;

	struct
	{
		mrb_sym x, y, z;
		mrb_sym position, angle;
	} sym_iv;

	struct
	{
		mrb_sym delta, frame;
		mrb_sym a, b, x, y;
		mrb_sym lb, rb;
		mrb_sym view, menu, guide;
		mrb_sym ls, rs;
		mrb_sym pad_h, pad_v;
		mrb_sym la_h, la_v, la_t;
		mrb_sym ra_h, ra_v, ra_t;
	} sym_cv;
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

static mrb_value sVmCallCos(mrb_state* mrb, mrb_value obj)
{
	// https://github.com/mruby/mruby/blob/master/mrbgems/mruby-math/src/math.c
	mrb_float x;

	mrb_get_args(mrb, "f", &x);
	x = cos(x);

	return mrb_float_value(mrb, x);
}


static mrb_value sVmCallSin(mrb_state* mrb, mrb_value obj)
{
	// https://github.com/mruby/mruby/blob/master/mrbgems/mruby-math/src/math.c
	mrb_float x;

	mrb_get_args(mrb, "f", &x);
	x = sin(x);

	return mrb_float_value(mrb, x);
}


/*-----------------------------

 sException()
 - Inspects if an exception was raised
 - (TODO) Prints an un-usefull message
 - (TODO) And Im not really sure if the VM is usable after this state
-----------------------------*/
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


/*-----------------------------

 sCheckVariableIn()
 - Checks if variable 'symbol' exists on 'object'
 - If true, return the variable in 'out'
 - If true, check if is of the specified 'type'
 - Returns 'true' if both previous conditions meet
-----------------------------*/
static bool sCheckVariableIn(struct mrb_state* state, struct mrb_value object, mrb_sym symbol, enum mrb_vtype type,
                             mrb_value* out)
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


/*-----------------------------

 sCheckVector3In()
 - Checks if variable 'symbol' in 'object' meets some conditions to pass as an Nara 'jaVector3'
 - If is an MRuby object
 - If has variables 'x', 'y' and 'z' (without check the type of these)
-----------------------------*/
static bool sCheckVector3In(struct mrb_state* state, struct mrb_value object, mrb_sym symbol)
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


/*-----------------------------

 sCheckNEntityClass()
 - If a MRuby class of 'class_name' exists returns it in 'out', without considering following tests
 - Checks if that MRuby class meets some conditions to pass as an Nara 'NEntityClass', looks if
 certain variables exist at initialization, math expected types, etc
 - It just creates an instance a do some checks on it, of course being a dynamic language is
 easy that those variables change of type or just dissapear on subsequent calls... in any
 case is better to print some warnings at execution that in the middle of the game
-----------------------------*/
static bool sCheckNEntityClass(struct mrb_state* state, const char* class_name, struct RClass** out)
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

	if (out != NULL)
		*out = class;

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
		valid = false;

	// TODO: Check if method 'think' exists

	// Bye!
	mrb_gc_mark_value(state, object);
	return valid;

return_failure:
	if (object.value.p != NULL)
		mrb_gc_mark_value(state, object);

	return false;
}


static int sCreateSymbols(struct Vm* vm)
{
	vm->sym_iv.x = mrb_intern_cstr(vm->rstate, "@x");
	vm->sym_iv.y = mrb_intern_cstr(vm->rstate, "@y");
	vm->sym_iv.z = mrb_intern_cstr(vm->rstate, "@z");
	vm->sym_iv.position = mrb_intern_cstr(vm->rstate, "@position");
	vm->sym_iv.angle = mrb_intern_cstr(vm->rstate, "@angle");

	vm->sym_cv.delta = mrb_intern_cstr(vm->rstate, "@@delta");
	vm->sym_cv.frame = mrb_intern_cstr(vm->rstate, "@@frame");
	vm->sym_cv.a = mrb_intern_cstr(vm->rstate, "@@a");
	vm->sym_cv.b = mrb_intern_cstr(vm->rstate, "@@b");
	vm->sym_cv.x = mrb_intern_cstr(vm->rstate, "@@x");
	vm->sym_cv.y = mrb_intern_cstr(vm->rstate, "@@y");
	vm->sym_cv.lb = mrb_intern_cstr(vm->rstate, "@@lb");
	vm->sym_cv.rb = mrb_intern_cstr(vm->rstate, "@@rb");
	vm->sym_cv.view = mrb_intern_cstr(vm->rstate, "@@view");
	vm->sym_cv.menu = mrb_intern_cstr(vm->rstate, "@@menu");
	vm->sym_cv.guide = mrb_intern_cstr(vm->rstate, "@@guide");
	vm->sym_cv.ls = mrb_intern_cstr(vm->rstate, "@@ls");
	vm->sym_cv.rs = mrb_intern_cstr(vm->rstate, "@@rs");
	vm->sym_cv.pad_h = mrb_intern_cstr(vm->rstate, "@@pad_h");
	vm->sym_cv.pad_v = mrb_intern_cstr(vm->rstate, "@@pad_v");
	vm->sym_cv.la_h = mrb_intern_cstr(vm->rstate, "@@la_h");
	vm->sym_cv.la_v = mrb_intern_cstr(vm->rstate, "@@la_v");
	vm->sym_cv.la_t = mrb_intern_cstr(vm->rstate, "@@la_t");
	vm->sym_cv.ra_h = mrb_intern_cstr(vm->rstate, "@@ra_h");
	vm->sym_cv.ra_v = mrb_intern_cstr(vm->rstate, "@@ra_v");
	vm->sym_cv.ra_t = mrb_intern_cstr(vm->rstate, "@@ra_t");

	return 0; // TODO
}


static int sUpdateGlobals(struct Vm* vm, struct VmGlobals* globals, bool force_update)
{
	if (globals->delta != vm->last_globals.delta || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->nara_module, vm->sym_cv.delta, mrb_float_value(vm->rstate, globals->delta));

	if (globals->frame != vm->last_globals.frame || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->nara_module, vm->sym_cv.frame, mrb_fixnum_value(globals->frame));

	if (globals->a != vm->last_globals.a || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.a, mrb_bool_value(globals->a));

	if (globals->b != vm->last_globals.b || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.b, mrb_bool_value(globals->b));

	if (globals->x != vm->last_globals.x || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.x, mrb_bool_value(globals->x));

	if (globals->y != vm->last_globals.y || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.y, mrb_bool_value(globals->y));

	if (globals->lb != vm->last_globals.lb || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.lb, mrb_bool_value(globals->lb));

	if (globals->rb != vm->last_globals.rb || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.rb, mrb_bool_value(globals->rb));

	if (globals->view != vm->last_globals.view || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.view, mrb_bool_value(globals->view));

	if (globals->menu != vm->last_globals.menu || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.menu, mrb_bool_value(globals->menu));

	if (globals->guide != vm->last_globals.guide || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.guide, mrb_bool_value(globals->guide));

	if (globals->ls != vm->last_globals.ls || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.ls, mrb_bool_value(globals->ls));

	if (globals->rs != vm->last_globals.rs || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.rs, mrb_bool_value(globals->rs));

	if (globals->pad.h != vm->last_globals.pad.h || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.pad_h, mrb_float_value(vm->rstate, globals->pad.h));

	if (globals->pad.v != vm->last_globals.pad.v || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.pad_v, mrb_float_value(vm->rstate, globals->pad.v));

	if (globals->la.h != vm->last_globals.la.h || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.la_h, mrb_float_value(vm->rstate, globals->la.h));

	if (globals->la.v != vm->last_globals.la.v || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.la_v, mrb_float_value(vm->rstate, globals->la.v));

	if (globals->la.t != vm->last_globals.la.t || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.la_t, mrb_float_value(vm->rstate, globals->la.t));

	if (globals->ra.h != vm->last_globals.ra.h || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.ra_h, mrb_float_value(vm->rstate, globals->ra.h));

	if (globals->ra.v != vm->last_globals.ra.v || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.ra_v, mrb_float_value(vm->rstate, globals->ra.v));

	if (globals->ra.t != vm->last_globals.ra.t || force_update == true)
		mrb_mod_cv_set(vm->rstate, vm->input_module, vm->sym_cv.ra_t, mrb_float_value(vm->rstate, globals->ra.t));

	memcpy(&vm->last_globals, globals, sizeof(struct VmGlobals));
	return 0; // TODO
}


static int sDefineGlobals(struct Vm* vm)
{
	vm->nara_module = mrb_define_module(vm->rstate, "Nara");
	vm->input_module = mrb_define_module_under(vm->rstate, vm->nara_module, "Input");

	mrb_define_module_function(vm->rstate, vm->nara_module, "print", sVmCallPrint, MRB_ARGS_REQ(1));
	mrb_define_module_function(vm->rstate, vm->nara_module, "cos", sVmCallCos, MRB_ARGS_REQ(1));
	mrb_define_module_function(vm->rstate, vm->nara_module, "sin", sVmCallSin, MRB_ARGS_REQ(1));

	sUpdateGlobals(vm, &(struct VmGlobals){0}, true);

	mrb_load_string(vm->rstate, "\
	module Nara\n\
		VERSION = \"v0.4-alpha\"\n\
		VERSION_MAJOR = 0\n\
		VERSION_MINOR = 1\n\
		VERSION_PATCH = 0\n\
		VERSION_SUFFIX = \"alpha\"\n\
		PI = 3.14159265358979323846264338327950288\n\
		def delta; @@delta; end; module_function :delta\n\
		def frame; @@frame; end; module_function :frame\n\
		module Input\n\
			def a; @@a; end; module_function :a\n\
			def b; @@b; end; module_function :b\n\
			def x; @@x; end; module_function :x\n\
			def y; @@y; end; module_function :y\n\
			def lb; @@lb; end; module_function :lb\n\
			def rb; @@rb; end; module_function :rb\n\
			def view; @@view; end; module_function :view\n\
			def menu; @@menu; end; module_function :menu\n\
			def guide; @@guide; end; module_function :guide\n\
			def ls; @@ls; end; module_function :ls\n\
			def rs; @@rs; end; module_function :rs\n\
			def pad_h; @@pad_h; end; module_function :pad_h\n\
			def pad_v; @@pad_v; end; module_function :pad_v\n\
			def la_h; @@la_h; end; module_function :la_h\n\
			def la_v; @@la_v; end; module_function :la_v\n\
			def la_t; @@la_t; end; module_function :la_t\n\
			def ra_h; @@ra_h; end; module_function :ra_h\n\
			def ra_v; @@ra_v; end; module_function :ra_v\n\
			def ra_t; @@ra_t; end; module_function :ra_t\n\
		end\n\
	end");

	return 0; // TODO
}


static inline int sSetVector3(struct Vm* vm, struct mrb_value vector, struct jaVector3 value)
{
	mrb_iv_set(vm->rstate, vector, vm->sym_iv.x, mrb_float_value(vm->rstate, value.x));
	mrb_iv_set(vm->rstate, vector, vm->sym_iv.y, mrb_float_value(vm->rstate, value.y));
	mrb_iv_set(vm->rstate, vector, vm->sym_iv.z, mrb_float_value(vm->rstate, value.z));
}


static struct jaVector3 sGetVector3(struct Vm* vm, struct mrb_value vector)
{
	struct jaVector3 ret = {0};

	ret.x = mrb_float(mrb_iv_get(vm->rstate, vector, vm->sym_iv.x));
	ret.y = mrb_float(mrb_iv_get(vm->rstate, vector, vm->sym_iv.y));
	ret.z = mrb_float(mrb_iv_get(vm->rstate, vector, vm->sym_iv.z));

	return ret;
}


//-----------------------------


struct Vm* VmCreate(const char* filename, struct jaStatus* st)
{
	(void)st; // TODO

	FILE* file = NULL;
	struct Vm* vm = NULL;

	if ((vm = calloc(1, sizeof(struct Vm))) == NULL)
		return NULL;

	if ((vm->classes = jaDictionaryCreate(NULL)) == NULL)
		goto return_failure;

	if ((vm->rstate = mrb_open_core(mrb_default_allocf, NULL)) == NULL)
		goto return_failure;

	if (sCreateSymbols(vm) != 0)
		goto return_failure;

	if (sDefineGlobals(vm) != 0)
		goto return_failure;

	// Load bytecode
	if (filename == NULL)
		goto return_failure;

	if ((file = fopen(filename, "r")) == NULL)
		goto return_failure;

	mrb_load_irep_file(vm->rstate, file);
	fclose(file);

	if (sException(vm->rstate) == true)
	{
		printf("[Error] Exception raised at '%s' load procedure\n", filename);
		goto return_failure;
	}

	// Save the current GC state, then a call to mrb_gc_arena_restore()
	// will free everything created from here. Indicated for C created
	// objects that the GC didn't know how to handle
	// (https://github.com/mruby/mruby/blob/master/doc/guides/gc-arena-howto.mds)
	vm->rgc_state = mrb_gc_arena_save(vm->rstate);

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

	if (vm->classes != NULL)
		jaDictionaryDelete(vm->classes);

	if (vm->rstate != NULL)
		mrb_close(vm->rstate);

	jaListClean(&vm->entities);
	free(vm);
}


inline void VmClean(struct Vm* vm)
{
	mrb_gc_arena_restore(vm->rstate, vm->rgc_state);
}


struct NEntity* VmCreateEntity(struct Vm* vm, const char* class_name, struct NEntityState initial_state)
{
	struct NEntity* entity = NULL;
	struct NEntityClass* class = NULL;

	struct mrb_value object = {.value.p = NULL};

	union {
		struct jaDictionaryItem* d;
		struct jaListItem* l;
	} item;

	// Is an know class?
	if ((item.d = jaDictionaryGet(vm->classes, class_name)) != NULL)
		class = item.d->data;
	else
	{
		struct RClass* temp = NULL;

		printf("Validating class '%s'...\n", class_name);

		if (sCheckNEntityClass(vm->rstate, class_name, &temp) == false)
			return NULL;

		if ((item.d = jaDictionaryAdd(vm->classes, class_name, NULL, sizeof(struct NEntityClass))) == NULL)
			return NULL;

		class = item.d->data;
		class->rclass = temp;
	}

	// Create MRuby object / Nara counterpart
	object = mrb_obj_new(vm->rstate, class->rclass, 0, NULL);

	if ((item.l = jaListAdd(&vm->entities, NULL, sizeof(struct NEntity))) == NULL)
		return NULL;

	entity = item.l->data;
	entity->state = initial_state;
	entity->robject = object;

	sCheckVariableIn(vm->rstate, object, vm->sym_iv.position, MRB_TT_OBJECT, &entity->position_robject);
	sCheckVariableIn(vm->rstate, object, vm->sym_iv.angle, MRB_TT_OBJECT, &entity->angle_robject);

	sSetVector3(vm, entity->position_robject, initial_state.position);
	sSetVector3(vm, entity->angle_robject, initial_state.angle);

	// Bye!
	printf("Created entity '%s'\n", class_name);
	return entity;
}


inline const struct NEntityState* VmEntityState(const struct NEntity* entity)
{
	return &entity->state;
}


inline void VmDeleteEntity(struct Vm* vm, struct NEntity* entity)
{
	// TODO
	(void)vm;
	(void)entity;
}


void VmEntitiesUpdate(struct Vm* vm, struct VmGlobals* globals)
{
	struct jaListItem* item = NULL;
	struct NEntity* entity = NULL;

	if (globals != NULL)
		sUpdateGlobals(vm, globals, false);

	struct jaListState s = {0};
	s.start = vm->entities.first;
	s.reverse = false;

	while ((item = jaListIterate(&s)) != NULL)
	{
		entity = item->data;
		mrb_funcall(vm->rstate, entity->robject, "think", 0);

		if (sException(vm->rstate) == true)
			printf("[Error] Exception raised at '%s' 'think' call\n",
			       mrb_class_name(vm->rstate, mrb_obj_ptr(entity->robject)->c));

		entity->state.old_position = entity->state.position;
		entity->state.old_angle = entity->state.angle;

		entity->state.position = sGetVector3(vm, entity->position_robject);
		entity->state.angle = sGetVector3(vm, entity->angle_robject);

		if (sException(vm->rstate) == true)
			printf("[Error] Exception raised when retrieving variables from '%s'\n",
			       mrb_class_name(vm->rstate, mrb_obj_ptr(entity->robject)->c));
	}
}
