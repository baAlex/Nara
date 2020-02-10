/*-----------------------------

 [vm.h]
 - Alexander Brandt 2019-2020
-----------------------------*/

#ifndef VM_H
#define VM_H

	#include "japan-status.h"
	#include "japan-vector.h"

	struct VmGlobals
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

	struct NEntityState
	{
		struct jaVector3 position;
		struct jaVector3 angle;

		struct jaVector3 old_position;
		struct jaVector3 old_angle;
	};

	struct Vm* VmCreate(const char* filename, struct jaStatus* st);
	void VmDelete(struct Vm*);
	void VmClean(struct Vm*);

	struct NEntity* VmCreateEntity(struct Vm*, const char* class_name, struct NEntityState initial_state);
	const struct NEntityState* VmEntityState(const struct NEntity*);
	void VmDeleteEntity(struct Vm*, struct NEntity*);

	void VmEntitiesUpdate(struct Vm*, struct VmGlobals*);

#endif
