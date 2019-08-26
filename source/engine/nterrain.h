/*-----------------------------

 [nterrain.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef NTERRAIN_H
#define NTERRAIN_H

	#include "tree.h"
	#include "vector.h"
	#include "status.h"
	#include <stdint.h>

	#ifndef CONTEXT_H
	struct Vertex
	{
		struct Vector3 pos;
		struct Vector2 uv;
	};
	#endif

	enum NVerticesType
	{
		INHERITED_FROM_PARENT = 0,
		SHARED_WITH_CHILDRENS = 1,
		OWN_VERTICES = 2
	};

	struct NTerrainNode
	{
		struct Vector2 min;
		struct Vector2 max;
		float pattern_dimension;

		uint16_t* index;
		size_t index_no;

		struct Vertex* vertices;
		size_t vertices_no;

		enum NVerticesType vertices_type;
	};

	struct NTerrain
	{
		struct Tree* root;

		float dimension;
		float min_tile_dimension;
		float min_pattern_dimension;

		size_t steps;
		size_t tiles_no;
		size_t vertices_buffers_no;
	};

	struct NTerrain* NTerrainCreate(float dimension, float minimum_tile_dimension, int pattern_subdivisions,
	                                struct Status* st);
	void NTerrainDelete(struct NTerrain* terrain);

	void NTerrainIterate(struct NTerrain* terrain, struct Vector2 position, void (*callback)(struct NTerrainNode*, void*), void* extra_data);

#endif
