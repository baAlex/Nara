/*-----------------------------

 [nterrain.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef NTERRAIN_H
#define NTERRAIN_H

	#include <stdint.h>

	#include "buffer.h"
	#include "vector.h"
	#include "status.h"
	#include "context.h"

	enum NVerticesType
	{
		INHERITED_FROM_PARENT = 0,
		SHARED_WITH_CHILDRENS = 1,
		OWN_VERTICES = 2
	};

	struct NTerrain
	{
		struct NTerrainNode* root;

		float dimension;
		float elevation;
		float min_node_dimension;
		float min_pattern_dimension;

		size_t steps;
		size_t nodes_no;
		size_t vertices_buffers_no;

		struct Buffer buffer;
	};

	struct NTerrainNode
	{
		struct Vector3 min;
		struct Vector3 max;

		float pattern_dimension;

		enum NVerticesType vertices_type;
		struct Vertices vertices;
		struct Index index;

		struct NTerrainNode* next;
		struct NTerrainNode* children;
	};

	struct NTerrainView
	{
		struct Vector3 position;
		struct Vector3 angle;
		float max_distance;
		float fov;
	};

	struct NTerrainState
	{
		struct NTerrainNode* start; // Set before iterate

		struct NTerrainNode* actual;
		size_t depth;

		struct NTerrainNode* future_return;
		size_t future_depth;

		struct NTerrainNode* last_with_vertices;
	};

	struct NTerrain* NTerrainCreate(const char* heightmap, float elevation, float dimension, float minimum_node_dimension,
	                                int pattern_subdivisions, struct Status* st);
	void NTerrainDelete(struct NTerrain* terrain);

	struct NTerrainNode* NTerrainIterate(struct NTerrainState* state, struct Buffer* buffer, struct NTerrainView*);

	int NTerrainDraw(struct NTerrain* terrain, struct NTerrainView*);
	void NTerrainPrintInfo(struct NTerrain* terrain);

#endif
