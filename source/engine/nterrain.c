/*-----------------------------

MIT License

Copyright (c) 2019 Alexander Brandt

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

-------------------------------

 [nterrain.c]
 - Alexander Brandt 2019
-----------------------------*/

#include "nterrain.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


/*-----------------------------

 sGenerateVertices()
-----------------------------*/
static void sGenerateVertices(struct Vertex* vertices, struct Vector2 min, struct Vector2 max, float pattern_dimension)
{
	int patterns_no = (max.x - min.x) / pattern_dimension;
	float heightmap_step_x = 0;
	float heightmap_step_y = 0;

	for (int row = 0; row < (patterns_no + 1); row++)
	{
		for (int col = 0; col < (patterns_no + 1); col++)
		{
			vertices[col + (patterns_no + 1) * row].uv.x = heightmap_step_x;
			vertices[col + (patterns_no + 1) * row].uv.y = heightmap_step_y;

			vertices[col + (patterns_no + 1) * row].pos.x = min.x + (float)(col * pattern_dimension);
			vertices[col + (patterns_no + 1) * row].pos.y = min.y + (float)(row * pattern_dimension);
			vertices[col + (patterns_no + 1) * row].pos.z = 0.0;

			heightmap_step_x += (float)pattern_dimension / (float)(max.x - min.x);
		}

		heightmap_step_y += (float)pattern_dimension / (float)(max.x - min.x);
		heightmap_step_x = 0.0;
	}
}


/*-----------------------------

 sSubdivideTile()
-----------------------------*/
static void sSubdivideTile(struct Tree* item, float min_tile_dimension)
{
	struct NTerrainNode* node = item->data;
	struct NTerrainNode* new_node = NULL;
	struct Tree* new_item = NULL;

	float node_dimension = (node->max.x - node->min.x); // Always an square

	// TODO: With all the loss of precision, can the equal comparision fail? (I think that yes? no?)
	if ((node_dimension / 3.0) <= min_tile_dimension)
		return;

	for (int i = 0; i < 9; i++)
	{
		new_item = TreeCreate(item, NULL, sizeof(struct NTerrainNode));
		new_node = new_item->data;

		switch (i)
		{
		case 0: // North West
			new_node->min.x = node->min.x;
			new_node->min.y = node->min.y;
			new_node->max.x = node->min.x + node_dimension / 3.0;
			new_node->max.y = node->min.y + node_dimension / 3.0;
			break;
		case 1: // North
			new_node->min.x = node->min.x + node_dimension / 3.0;
			new_node->min.y = node->min.y;
			new_node->max.x = node->min.x + node_dimension / 3.0 * 2.0;
			new_node->max.y = node->min.y + node_dimension / 3.0;
			break;
		case 2: // North East
			new_node->min.x = node->min.x + node_dimension / 3.0 * 2.0;
			new_node->min.y = node->min.y;
			new_node->max.x = node->min.x + node_dimension / 3.0 * 3.0;
			new_node->max.y = node->min.y + node_dimension / 3.0;
			break;
		case 3: // West
			new_node->min.x = node->min.x;
			new_node->min.y = node->min.y + node_dimension / 3.0;
			new_node->max.x = node->min.x + node_dimension / 3.0;
			new_node->max.y = node->min.y + node_dimension / 3.0 * 2.0;
			break;
		case 4: // Center
			new_node->min.x = node->min.x + node_dimension / 3.0;
			new_node->min.y = node->min.y + node_dimension / 3.0;
			new_node->max.x = node->min.x + node_dimension / 3.0 * 2.0;
			new_node->max.y = node->min.y + node_dimension / 3.0 * 2.0;
			break;
		case 5: // East
			new_node->min.x = node->min.x + node_dimension / 3.0 * 2.0;
			new_node->min.y = node->min.y + node_dimension / 3.0;
			new_node->max.x = node->min.x + node_dimension / 3.0 * 3.0;
			new_node->max.y = node->min.y + node_dimension / 3.0 * 2.0;
			break;
		case 6: // South West
			new_node->min.x = node->min.x;
			new_node->min.y = node->min.y + node_dimension / 3.0 * 2.0;
			new_node->max.x = node->min.x + node_dimension / 3.0;
			new_node->max.y = node->min.y + node_dimension / 3.0 * 3.0;
			break;
		case 7: // South
			new_node->min.x = node->min.x + node_dimension / 3.0;
			new_node->min.y = node->min.y + node_dimension / 3.0 * 2.0;
			new_node->max.x = node->min.x + node_dimension / 3.0 * 2.0;
			new_node->max.y = node->min.y + node_dimension / 3.0 * 3.0;
			break;
		case 8: // South East
			new_node->min.x = node->min.x + node_dimension / 3.0 * 2.0;
			new_node->min.y = node->min.y + node_dimension / 3.0 * 2.0;
			new_node->max.x = node->min.x + node_dimension / 3.0 * 3.0;
			new_node->max.y = node->min.y + node_dimension / 3.0 * 3.0;
		}

		sSubdivideTile(new_item, min_tile_dimension);
	}
}


/*-----------------------------

 NTerrainCreate()
-----------------------------*/
struct NTerrain* NTerrainCreate(float dimension, float min_tile_dimension, int pattern_subdivisions, struct Status* st)
{
	struct NTerrain* terrain = NULL;
	struct NTerrainNode* node = NULL;

	struct Tree* item = NULL;
	struct Buffer buffer = {0};
	struct TreeState s = {0};

	StatusSet(st, "NTerraiNCreate", STATUS_SUCCESS, NULL);

	if ((terrain = calloc(1, sizeof(struct NTerrain))) == NULL ||
	    (terrain->root = TreeCreate(NULL, NULL, sizeof(struct NTerrainNode))) == NULL)
	{
		StatusSet(st, "NTerraiNCreate", STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	terrain->dimension = dimension;

	for (float i = dimension; i > min_tile_dimension; i /= 3.0)
	{
		terrain->min_tile_dimension = i;
		terrain->min_pattern_dimension = i;
		terrain->steps++;
	}

	for (int i = 0; i < pattern_subdivisions; i++)
		terrain->min_pattern_dimension /= 3.0;

	// Tile subdivision
	node = terrain->root->data;
	node->min = (struct Vector2){0.0, 0.0};
	node->max = (struct Vector2){dimension, dimension};

	sSubdivideTile(terrain->root, min_tile_dimension);

	// Pattern subdivision
	struct Tree* p = NULL;
	size_t p_depth = 0;

	s.start = terrain->root;

	while ((item = TreeIterate(&s, &buffer)) != NULL)
	{
		node = item->data;
		terrain->tiles_no++;

		node->pattern_dimension = (node->max.x - node->min.x);

		for (int i = 0; i < pattern_subdivisions; i++)
			node->pattern_dimension /= 3.0;

		// Any parent has vertices to share?
		if (p != NULL && s.depth > p_depth)
		{
			node->vertices_type = INHERITED_FROM_PARENT;
			node->vertices = NULL;
			node->vertices_no = 0;
		}
		else
		{
			// Can this node keep all childrens vertices? (because indices in OpenGL ES2)
			if (powf((node->max.x - node->min.x) / terrain->min_pattern_dimension, 2.0) < UINT16_MAX)
			{
				p = item;
				p_depth = s.depth;
				node->vertices_type = SHARED_WITH_CHILDRENS;
				terrain->vertices_buffers_no++;
			}

			// Nope, only his own vertices
			else
			{
				p = NULL;
				node->vertices_type = OWN_VERTICES;
				terrain->vertices_buffers_no++;
			}

			// Actual subdivision
			node->vertices_no = (size_t)ceil((node->max.x - node->min.x) / node->pattern_dimension + 1.0);
			node->vertices_no *= node->vertices_no;
			node->vertices = malloc(sizeof(struct Vertex) * node->vertices_no);

			sGenerateVertices(node->vertices, node->min, node->max, node->pattern_dimension);
		}
	}

	// Bye!
	return terrain;

return_failure:
	if (terrain != NULL)
		NTerrainDelete(terrain);
	return NULL;
}


/*-----------------------------

 NTerrainDelete()
-----------------------------*/
void NTerrainDelete(struct NTerrain* terrain)
{
	struct TreeState s = {.start = terrain->root};
	struct Buffer buffer = {0};
	struct Tree* item = NULL;
	struct NTerrainNode* node = NULL;

	if (terrain->root != NULL)
	{
		while ((item = TreeIterate(&s, &buffer)) != NULL)
		{
			node = item->data;

			if (node->vertices != NULL)
				free(node->vertices);
		}

		TreeDelete(terrain->root);
	}

	free(terrain);
}
