/*-----------------------------

 [vm.h]
 - Alexander Brandt 2019-2020
-----------------------------*/

#ifndef VM_H
#define VM_H

	#include "japan-status.h"
	#include "japan-vector.h"

	struct Globals
	{
		bool a, b, x, y;
		bool lb, rb;
		bool view, menu, guide;
		bool ls, rs;

		struct { float h, v; } pad;
		struct { float h, v, t; } la;
		struct { float h, v, t; } ra;

		float delta;
		long frame;
	};

	struct Entity
	{
		struct jaVector3 position;
		struct jaVector3 angle;

		struct jaVector3 old_position;
		struct jaVector3 old_angle;
	};

	struct Vm* VmCreate(const char* filename, struct jaStatus* st);
	void VmDelete(struct Vm*);
	void VmClean(struct Vm*);

	struct Entity* VmCreateEntity(struct Vm*, const char* class_name, struct Entity initial_state, struct jaStatus* st);
	void VmDeleteEntity(struct Vm*, struct Entity*);

	int VmEntitiesUpdate(struct Vm*, struct Globals*, struct jaStatus* st);

#endif
