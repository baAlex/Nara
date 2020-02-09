/*-----------------------------

 [vm.h]
 - Alexander Brandt 2019-2020
-----------------------------*/

#ifndef VM_H
#define VM_H

	#include "japan-status.h"
	#include "japan-vector.h"

	struct NEntityState
	{
		struct jaVector3 position;
		struct jaVector3 angle;
	};

	struct Vm* VmCreate(const char* filename[], struct jaStatus* st);
	void VmDelete(struct Vm*);
	void VmClean(struct Vm*);

	struct NEntity* VmCreateEntity(struct Vm*, const char* class_name, struct NEntityState initial_state);
	void VmDeleteEntity(struct Vm*, struct NEntity*);

	void VmEntitiesUpdate(struct Vm*);

#endif
