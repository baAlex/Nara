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

#include "private.h"


static bool sCheckVariableInObject(struct mrb_state* state, struct mrb_value object, mrb_sym symbol,
                                   enum mrb_vtype type, mrb_value* out)
{
	mrb_value temp = {.value.p = NULL};

	if (mrb_iv_defined(state, object, symbol) == 0) // 0 == false
		return false;
	else
	{
		temp = mrb_iv_get(state, object, symbol);

		if (mrb_type(temp) != type)
			return false;
	}

	if (out != NULL)
		*out = temp;

	return true;
}


static bool sCheckVectorInObject(struct mrb_state* state, struct mrb_value object, mrb_sym symbol, mrb_value* out)
{
	mrb_value temp = {.value.p = NULL};
	const char* class_name = mrb_class_name(state, mrb_obj_ptr(object)->c);

	bool valid = true;

	if (sCheckVariableInObject(state, object, symbol, MRB_TT_OBJECT, &temp) == false)
	{
		printf("[MRuby error] '%s' didn't include an object '%s' definition\n", class_name,
		       mrb_sym2name(state, symbol));
		valid = false;
	}
	else
	{
		if (mrb_iv_defined(state, temp, mrb_intern_cstr(state, "@x")) == 0) // 0 == false
		{
			printf("[MRuby error] Variable '@x' not defined in '%s' '%s' object\n", class_name,
			       mrb_sym2name(state, symbol));
			valid = false;
		}

		if (mrb_iv_defined(state, temp, mrb_intern_cstr(state, "@y")) == 0)
		{
			printf("[MRuby error] Variable '@y' not defined in '%s' '%s' object\n", class_name,
			       mrb_sym2name(state, symbol));
			valid = false;
		}

		if (mrb_iv_defined(state, temp, mrb_intern_cstr(state, "@z")) == 0)
		{
			printf("[MRuby error] Variable '@z' not defined in '%s' '%s' object\n", class_name,
			       mrb_sym2name(state, symbol));
			valid = false;
		}
	}

	if (out != NULL)
		*out = temp;

	return valid;
}


static void sSetVector(struct Vm* vm, struct mrb_value vector, struct jaVector3 value)
{
	mrb_iv_set(vm->rstate, vector, vm->sym_iv.x, mrb_float_value(vm->rstate, value.x));
	mrb_iv_set(vm->rstate, vector, vm->sym_iv.y, mrb_float_value(vm->rstate, value.y));
	mrb_iv_set(vm->rstate, vector, vm->sym_iv.z, mrb_float_value(vm->rstate, value.z));
}


static struct jaVector3 sGetVector(struct Vm* vm, struct mrb_value vector)
{
	struct jaVector3 ret = {0};

	ret.x = (float)mrb_float(mrb_iv_get(vm->rstate, vector, vm->sym_iv.x));
	ret.y = (float)mrb_float(mrb_iv_get(vm->rstate, vector, vm->sym_iv.y));
	ret.z = (float)mrb_float(mrb_iv_get(vm->rstate, vector, vm->sym_iv.z));

	return ret;
}


static bool sValidateClass(struct mrb_state* state, const char* class_name, struct RClass** out)
{
	struct RClass* class = NULL;
	struct mrb_value object = {.value.p = NULL};

	bool valid = true;

	if (mrb_class_defined(state, class_name) == 0)
	{
		printf("[MRuby error] Class '%s' not defined\n", class_name);
		return false;
	}

	class = mrb_class_get(state, class_name); // Always after mrb_class_defined()
	object = mrb_obj_new(state, class, 0, NULL);

	if (CheckException(state) == true)
		goto return_failure;

	// Check definitions needed by the engine
	if (sCheckVectorInObject(state, object, mrb_intern_cstr(state, "@position"), NULL) == false)
		valid = false;

	if (sCheckVectorInObject(state, object, mrb_intern_cstr(state, "@angle"), NULL) == false)
		valid = false;

	// TODO: Check if method 'think' exists

	// Bye!
	if (out != NULL)
		*out = class;

	mrb_gc_mark_value(state, object);
	return valid;

return_failure:
	if (object.value.p != NULL)
		mrb_gc_mark_value(state, object);

	return false;
}


// ----


inline bool CheckException(struct mrb_state* state)
{
	if (state->exc != NULL)
	{
		printf("[MRuby error] ");
		mrb_print_error(state);

		state->exc = NULL;
		return true;
	}

	return false;
}


struct Vm* VmCreate(const char* filename, struct jaStatus* st)
{
	FILE* file = NULL;
	struct Vm* vm = NULL;

	jaStatusSet(st, "VmCreate", STATUS_SUCCESS, NULL);

	if (filename == NULL)
	{
		jaStatusSet(st, "VmCreate", STATUS_INVALID_ARGUMENT, NULL);
		goto return_failure;
	}

	if ((vm = calloc(1, sizeof(struct Vm))) == NULL || (vm->classes = jaDictionaryCreate(NULL)) == NULL)
	{
		jaStatusSet(st, "VmCreate", STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	if ((vm->rstate = mrb_open_core(mrb_default_allocf, NULL)) == NULL)
	{
		jaStatusSet(st, "VmCreate", STATUS_ERROR, "Initializing MRuby");
		goto return_failure;
	}

	// Definitions
	vm->sym_iv.x = mrb_intern_cstr(vm->rstate, "@x");
	vm->sym_iv.y = mrb_intern_cstr(vm->rstate, "@y");
	vm->sym_iv.z = mrb_intern_cstr(vm->rstate, "@z");
	vm->sym_iv.position = mrb_intern_cstr(vm->rstate, "@position");
	vm->sym_iv.angle = mrb_intern_cstr(vm->rstate, "@angle");

	CreateSymbols(vm);
	DefineGlobals(vm);

	if (CheckException(vm->rstate) == true)
	{
		jaStatusSet(st, "VmCreate", STATUS_ERROR, "Exception raised at definitions creation");
		goto return_failure;
	}

	// Load the bytecode
	if ((file = fopen(filename, "r")) == NULL)
	{
		jaStatusSet(st, "vmCreate", STATUS_IO_ERROR, NULL);
		goto return_failure;
	}

	mrb_load_irep_file(vm->rstate, file);

	if (CheckException(vm->rstate) == true)
	{
		jaStatusSet(st, "VmCreate", STATUS_ERROR, "Exception raised at '%s' loading", filename);
		goto return_failure;
	}

	// Save the current GC state, then a call to mrb_gc_arena_restore()
	// will free everything created from here. Indicated for C created
	// objects that the GC didn't know how to handle
	// (https://github.com/mruby/mruby/blob/master/doc/guides/gc-arena-howto.mds)
	vm->rgc_state = mrb_gc_arena_save(vm->rstate);

	// Bye!
	fclose(file);
	return vm;

return_failure:
	if (file != NULL)
		fclose(file);
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


void VmClean(struct Vm* vm)
{
	mrb_gc_arena_restore(vm->rstate, vm->rgc_state);
}


struct Entity* VmCreateEntity(struct Vm* vm, const char* class_name, struct Entity initial_state, struct jaStatus* st)
{
	struct VmEntity* entity = NULL;
	struct VmClass* class = NULL;

	struct mrb_value object = {.value.p = NULL};

	union {
		struct jaDictionaryItem* d;
		struct jaListItem* l;
	} item;

	jaStatusSet(st, "VmCreateEntity", STATUS_SUCCESS, NULL);

	// Is an know class?
	if ((item.d = jaDictionaryGet(vm->classes, class_name)) != NULL)
		class = item.d->data;
	else
	{
		struct RClass* temp = NULL;

		printf("Validating class '%s'...\n", class_name);

		if (sValidateClass(vm->rstate, class_name, &temp) == false)
		{
			jaStatusSet(st, "VmCreateEntity", STATUS_ERROR,
			            "MRuby class '%s' didn't meets the conditions to be a valid game entity class", class_name);
			return NULL;
		}

		if ((item.d = jaDictionaryAdd(vm->classes, class_name, NULL, sizeof(struct VmClass))) == NULL)
		{
			jaStatusSet(st, "VmCreateEntity", STATUS_MEMORY_ERROR, NULL);
			return NULL;
		}

		class = item.d->data;
		class->rclass = temp;
	}

	// Create MRuby object / Nara counterpart
	object = mrb_obj_new(vm->rstate, class->rclass, 0, NULL);

	if ((item.l = jaListAdd(&vm->entities, NULL, sizeof(struct VmEntity))) == NULL)
	{
		jaStatusSet(st, "VmCreateEntity", STATUS_MEMORY_ERROR, NULL);
		return NULL;
	}

	entity = item.l->data;
	entity->state = initial_state;
	entity->robject = object;

	sCheckVectorInObject(vm->rstate, object, vm->sym_iv.position, &entity->position_robject);
	sCheckVectorInObject(vm->rstate, object, vm->sym_iv.angle, &entity->angle_robject);

	sSetVector(vm, entity->position_robject, initial_state.position);
	sSetVector(vm, entity->angle_robject, initial_state.angle);

	// Bye!
	printf("Created entity '%s'\n", class_name);
	return (struct Entity*)entity; // A lie!
}


inline void VmDeleteEntity(struct Vm* vm, struct Entity* fake_entity)
{
	struct VmEntity* entity = (struct VmEntity*)fake_entity;

	// TODO
	(void)vm;
	(void)entity;
}


int VmEntitiesUpdate(struct Vm* vm, struct Globals* globals, struct jaStatus* st)
{
	struct jaListItem* item = NULL;
	struct VmEntity* entity = NULL;

	jaStatusSet(st, "VmEntitiesUpdate", STATUS_SUCCESS, NULL);

	if (globals != NULL)
		UpdateGlobals(vm, globals, false);

	struct jaListState s = {0};
	s.start = vm->entities.first;
	s.reverse = false;

	while ((item = jaListIterate(&s)) != NULL)
	{
		entity = item->data;
		mrb_funcall(vm->rstate, entity->robject, "think", 0);

		if (CheckException(vm->rstate) == true)
		{
			jaStatusSet(st, "VmEntitiesUpdate", STATUS_ERROR, "Exception raised at '%s' 'think'call",
			            mrb_class_name(vm->rstate, mrb_obj_ptr(entity->robject)->c));
			return 1;
		}

		entity->state.old_position = entity->state.position;
		entity->state.old_angle = entity->state.angle;

		entity->state.position = sGetVector(vm, entity->position_robject);
		entity->state.angle = sGetVector(vm, entity->angle_robject);

		if (CheckException(vm->rstate) == true)
		{
			jaStatusSet(st, "VmEntitiesUpdate", STATUS_ERROR, "Exception raised while retrieving variables from '%s'",
			            mrb_class_name(vm->rstate, mrb_obj_ptr(entity->robject)->c));
			return 1;
		}
	}

	return 0;
}
