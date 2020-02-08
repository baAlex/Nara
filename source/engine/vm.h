/*-----------------------------

 [vm.h]
 - Alexander Brandt 2019-2020
-----------------------------*/

#ifndef VM_H
#define VM_H

	#include "japan-status.h"
	#include "japan-vector.h"

	struct EntityState
	{
		struct jaVector3 position;
		struct jaVector3 angle;
	};

	struct Vm* VmCreate(const char* filename[], struct jaStatus* st);
	void VmDelete(struct Vm*);

#endif
