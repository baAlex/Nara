/*-----------------------------

 [vm/private.h]
 - Alexander Brandt 2019-2020
-----------------------------*/

#ifndef VM_PRIVATE_H
#define VM_PRIVATE_H

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

	struct VmClass
	{
		struct RClass* rclass;
	};

	struct VmEntity
	{
		struct Entity state; // What is visible to the exterior

		struct mrb_value robject;
		struct mrb_value position_robject;
		struct mrb_value angle_robject;
	};

	struct Vm
	{
		struct jaDictionary* classes;
		struct jaList entities;
		struct Globals globals;

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

	bool CheckException(struct mrb_state* state);

	void CreateSymbols(struct Vm* vm);
	void DefineGlobals(struct Vm* vm);
	void UpdateGlobals(struct Vm* vm, struct Globals* new_globals, bool force_update);

#endif
