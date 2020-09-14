/*-----------------------------

 [skydome.h]
 - Alexander Brandt 2020
-----------------------------*/

#ifndef SKYDOME_H
#define SKYDOME_H

#include "japan-buffer.h"
#include "japan-status.h"
#include "kansai-context.h"

struct Skydome
{
	struct kaVertices vertices;
	struct kaIndex index;
};

struct Skydome* SkydomeCreate(struct kaWindow* w, float radius, int iterations, struct jaBuffer* temp_buffer,
                              struct jaStatus* st);
void SkydomeDelete(struct kaWindow* w, struct Skydome*);

#endif
