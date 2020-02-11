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

 [definitions.c]
 - Alexander Brandt 2019-2020
-----------------------------*/

#include "private.h"


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


mrb_value VmCallCos(mrb_state* mrb, mrb_value self)
{
	// https://github.com/mruby/mruby/blob/master/mrbgems/mruby-math/src/math.c
	(void)self;
	mrb_float x;

	mrb_get_args(mrb, "f", &x);
	x = cos(x);

	return mrb_float_value(mrb, x);
}


mrb_value VmCallSin(mrb_state* mrb, mrb_value self)
{
	// https://github.com/mruby/mruby/blob/master/mrbgems/mruby-math/src/math.c
	(void)self;
	mrb_float x;

	mrb_get_args(mrb, "f", &x);
	x = sin(x);

	return mrb_float_value(mrb, x);
}


void CreateSymbols(struct Vm* vm)
{
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
}


void DefineGlobals(struct Vm* vm)
{
	vm->nara_module = mrb_define_module(vm->rstate, "Nara");
	vm->input_module = mrb_define_module_under(vm->rstate, vm->nara_module, "Input");

	mrb_define_module_function(vm->rstate, vm->nara_module, "print", VmCallPrint, MRB_ARGS_REQ(1));
	mrb_define_module_function(vm->rstate, vm->nara_module, "cos", VmCallCos, MRB_ARGS_REQ(1));
	mrb_define_module_function(vm->rstate, vm->nara_module, "sin", VmCallSin, MRB_ARGS_REQ(1));

	UpdateGlobals(vm, &(struct Globals){0}, true);

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
}


void UpdateGlobals(struct Vm* vm, struct Globals* globals, bool force_update)
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

	memcpy(&vm->last_globals, globals, sizeof(struct Globals));
}
