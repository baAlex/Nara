/*-----------------------------

 [vm.h]
 - Alexander Brandt 2020
-----------------------------*/

#ifndef VM_H
#define VM_H

#include "japan-status.h"

struct Vm* VmCreate(const char* main_filename, struct jaStatus* st);
void VmDelete(struct Vm*);

int VmCallbackInit(struct Vm*, struct jaStatus*);
int VmCallbackFrame(struct Vm*, float delta, struct jaStatus*);
int VmCallbackResize(struct Vm*, int width, int height, struct jaStatus*);
int VmCallbackFunction(struct Vm*, int f, struct jaStatus*);
int VmCallbackQuit(struct Vm*);

#endif
