/*-----------------------------

 [terrain.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef TERRAIN_H
#define TERRAIN_H

	#include "vector.h"
	#include "status.h"
	#include "context-gl.h"

	struct Terrain
	{
		size_t width;
		size_t height;

		struct Vertices* vertices;
		struct Index* index;
	};

	int TerrainLoad(struct Terrain*, size_t width, size_t height, const char* heightmap_filename, struct Status* st);
	void TerrainClean(struct Terrain*);

#endif
