/*-----------------------------

 [nterrain.h]
 - Alexander Brandt 2019-2020
-----------------------------*/

#ifndef NTERRAIN_H
#define NTERRAIN_H

	#include <stdint.h>

	#include "japan-aabounding.h"
	#include "japan-buffer.h"
	#include "japan-image.h"
	#include "japan-status.h"
	#include "japan-vector.h"

	#include "context/context.h"

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

		struct jaImage* heightmap;

		struct jaBuffer buffer;
	};

	struct NTerrainNode
	{
		struct jaAABBox bbox;

		float pattern_dimension;

		enum NVerticesType vertices_type;
		struct Vertices vertices;
		struct Index index;

		struct NTerrainNode* next;
		struct NTerrainNode* children;
		struct NTerrainNode* parent;
	};

	struct NTerrainView
	{
		struct jaVector3 position;
		struct jaVector3 angle;
		float max_distance;
		float aspect;
		float fov;
		int lod_factor; // 1, 3, 6, 9... in steps of 3
	};

	struct NTerrainState
	{
		struct NTerrainNode* start; // Set before iterate

		struct NTerrainNode* actual;
		size_t depth;

		struct NTerrainNode* future_return;
		size_t future_depth;

		struct NTerrainNode* last_with_vertices;

		struct jaVector3 frustum_position[5];
		struct jaVector3 frustum_normal[5];

		struct jaAABRectangle view_rectangle;
		bool in_border;
	};

	struct NTerrain* NTerrainCreate(const char* heightmap, float elevation, float dimension, float minimum_node_dimension,
	                                int pattern_subdivisions, struct jaStatus* st);
	void NTerrainDelete(struct NTerrain* terrain);

	struct NTerrainNode* NTerrainIterate(struct NTerrainState* state, struct jaBuffer* buffer, struct NTerrainView*);

	int NTerrainDraw(struct Context*, struct NTerrain* terrain, struct NTerrainView*);
	void NTerrainPrintInfo(struct NTerrain* terrain);

	float NTerrainElevation(const struct NTerrain*, struct jaVector2);

#endif
