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
	#include "context.h"

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
		struct Vertices vertices;
		struct Index index;
	};

	struct NTerrain
	{
		struct Tree* root;

		float dimension;
		float elevation;
		float min_tile_dimension;
		float min_pattern_dimension;

		size_t steps;
		size_t tiles_no;
		size_t vertices_buffers_no;

		struct Buffer buffer;
	};

	struct NTerrain* NTerrainCreate(const char* heightmap, float elevation, float dimension, float minimum_tile_dimension,
	                                int pattern_subdivisions, struct Status* st);
	void NTerrainDelete(struct NTerrain* terrain);

	struct NTerrainNode* NTerrainIterate(struct TreeState* state, struct Buffer* buffer,
	                                     struct NTerrainNode** out_with_vertices, struct Vector3 camera_position);
	int NTerrainDraw(struct NTerrain* terrain, float max_distance, struct Vector3 cam_position, struct Vector3 cam_angle);
	void NTerrainPrintInfo(const struct NTerrain* terrain);

#endif
