/*-----------------------------

 [nterrain.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef NTERRAIN_H
#define NTERRAIN_H

	#include <stdint.h>

	#include "tree.h"
	#include "vector.h"
	#include "status.h"

	#ifdef TEST
	struct Vertex
	{
		struct Vector3 pos;
		struct Vector2 uv;
	};
	#else
	#include "context.h"
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
		enum NVerticesType vertices_type;

		#ifdef TEST
			uint16_t* index;
			size_t index_no;
			struct Vertex* vertices;
			size_t vertices_no;
		#else
			struct Vertices* vertices;
			struct Index* index;
		#endif

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

	struct NTerrain* NTerrainCreate(float dimension, float minimum_tile_dimension, int pattern_subdivisions, struct Status* st);
	void NTerrainDelete(struct NTerrain* terrain);

	struct NTerrainNode* NTerrainIterate(struct TreeState* state, struct Buffer* buffer, struct NTerrainNode** out_shared, struct Vector2 position);

#endif
