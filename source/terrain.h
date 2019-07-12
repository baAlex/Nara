/*-----------------------------

 [terrain.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef TERRAIN_H
#define TERRAIN_H

	#include "context-gl.h"

	struct TerrainOptions
	{
		size_t width;
		size_t height;
		size_t elevation;

		char* heightmap_filename;
		char* colormap_filename;
	};

	struct Terrain
	{
		struct TerrainOptions options;
		struct Image* heightmap;

		// GL specific objects
		struct Vertices* vertices;
		struct Index* index;

		struct Texture* color;
	};

	struct Terrain* TerrainCreate(struct TerrainOptions options, struct Status* st);
	void TerrainDelete(struct Terrain* terrain);

#endif
