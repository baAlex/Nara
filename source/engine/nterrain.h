/*-----------------------------

 [nterrain.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef NTERRAIN_H
#define NTERRAIN_H

	#include "tree.h"
	#include "vector.h"
	#include "image.h"
	#include "status.h"

	struct NTerrainNode
	{
		struct Vector2 min;
		struct Vector2 max;

		struct Buffer index;
		struct Buffer vertices;

		struct
		{
			bool has_vertices:1;
		};
	};

	struct NTerrain
	{
		struct Tree* root;

		float dimension;
		float min_tile_dimension;
		float min_pattern_dimension;

		size_t steps;
	};

	struct NTerrain* NTerrainCreate(float dimension, float minimum_tile_dimension, float minimum_pattern_dimension, struct Status* st);
	void NTerrainDelete(struct NTerrain* terrain);

#endif
