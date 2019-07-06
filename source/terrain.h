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
		int width;
		int height;

		struct Vertices* vertices;
		struct Index* index;
	};

	int TerrainLoad(struct Terrain*, int width, int height, const char* heightmap_filename, struct Status* st);
	void TerrainClean(struct Terrain*);

	struct Vertices* TerrainGetVertices(struct Terrain* terrain);
	struct Index* TerrainGetIndex(struct Terrain* terrain, int lod_level, struct Vector3 area_start, struct Vector3 area_end);

#endif
