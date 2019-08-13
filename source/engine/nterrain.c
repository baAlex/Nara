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
#include <time.h>


/*-----------------------------

 NTerrainCreate()
-----------------------------*/
static void sSubdivideTile(struct Tree* item, float min_tile_dimension)
{
	struct NTerrainNode* node = item->data;
	struct NTerrainNode* new_node = NULL;
	struct Tree* new_item = NULL;

	float node_dimension = (node->max.x - node->min.x); // Always an square

	if ((node_dimension / 3.0) < min_tile_dimension)
		return;

	for (int i = 0; i < 9; i++)
	{
		new_item = TreeCreate(item, NULL, sizeof(struct NTerrainNode));
		new_node = new_item->data;

		switch (i)
		{
		case 0: // North West
			new_node->min = (struct Vector2){node->min.x, node->min.y};
			new_node->max = (struct Vector2){node->min.x + node_dimension / 3.0, node->min.y + node_dimension / 3.0};
			break;
		case 1: // North
			new_node->min = (struct Vector2){node->min.x + node_dimension / 3.0, node->min.y};
			new_node->max = (struct Vector2){node->min.x + node_dimension / 3.0 * 2.0, node->min.y + node_dimension / 3.0};
			break;
		case 2: // North East
			new_node->min = (struct Vector2){node->min.x + node_dimension / 3.0 * 2.0, node->min.y};
			new_node->max = (struct Vector2){node->min.x + node_dimension / 3.0 * 3.0, node->min.y + node_dimension / 3.0};
			break;
		case 3: // West
			new_node->min = (struct Vector2){node->min.x, node->min.y + node_dimension / 3.0};
			new_node->max = (struct Vector2){node->min.x + node_dimension / 3.0, node->min.y + node_dimension / 3.0 * 2.0};
			break;
		case 4: // Center
			new_node->min = (struct Vector2){node->min.x + node_dimension / 3.0, node->min.y + node_dimension / 3.0};
			new_node->max = (struct Vector2){node->min.x + node_dimension / 3.0 * 2.0, node->min.y + node_dimension / 3.0 * 2.0};
			break;
		case 5: // East
			new_node->min =
			    (struct Vector2){node->min.x + node_dimension / 3.0 * 2.0, node->min.y + node_dimension / 3.0};
			new_node->max = (struct Vector2){node->min.x + node_dimension / 3.0 * 3.0, node->min.y + node_dimension / 3.0 * 2.0};
			break;
		case 6: // South West
			new_node->min = (struct Vector2){node->min.x, node->min.y + node_dimension / 3.0 * 2.0};
			new_node->max = (struct Vector2){node->min.x + node_dimension / 3.0, node->min.y + node_dimension / 3.0 * 3.0};
			break;
		case 7: // South
			new_node->min = (struct Vector2){node->min.x + node_dimension / 3.0, node->min.y + node_dimension / 3.0 * 2.0};
			new_node->max = (struct Vector2){node->min.x + node_dimension / 3.0 * 2.0, node->min.y + node_dimension / 3.0 * 3.0};
			break;
		case 8: // South East
			new_node->min = (struct Vector2){node->min.x + node_dimension / 3.0 * 2.0, node->min.y + node_dimension / 3.0 * 2.0};
			new_node->max = (struct Vector2){node->min.x + node_dimension / 3.0 * 3.0, node->min.y + node_dimension / 3.0 * 3.0};
		}

		sSubdivideTile(new_item, min_tile_dimension);
	}
}

struct NTerrain* NTerrainCreate(float dimension, float min_tile_dimension, float min_pattern_dimension,
                                struct Status* st)
{
	struct NTerrain* terrain = NULL;
	struct NTerrainNode* node = NULL;

	struct Tree* item = NULL;
	struct Buffer buffer = {0};
	struct TreeState s = {0};

	struct timespec start_time = {0};
	struct timespec end_time = {0};
	double ms = 0.0;

	timespec_get(&start_time, TIME_UTC);
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
		terrain->steps++;
	}

	// Tile subdivision
	node = terrain->root->data;
	node->min = (struct Vector2){0.0, 0.0};
	node->max = (struct Vector2){dimension, dimension};

	sSubdivideTile(terrain->root, min_tile_dimension); // TODO: Check error

	// Pattern subdivision
	s.start = terrain->root;

	while ((item = TreeIterate(&s, &buffer)) != NULL)
	{
		node = item->data;
	}

	// Bye!
	timespec_get(&end_time, TIME_UTC);
	ms = end_time.tv_nsec / 1000000.0 + end_time.tv_sec * 1000.0;
	ms -= start_time.tv_nsec / 1000000.0 + start_time.tv_sec * 1000.0;

	printf("Done in %0.4f ms\n", ms);
	return terrain;

return_failure:
	if (terrain != NULL)
		NTerrainDelete(terrain);
	return NULL;
}


/*-----------------------------

 NTerrainDelete()
-----------------------------*/
inline void NTerrainDelete(struct NTerrain* terrain)
{
	if (terrain->root != NULL)
		TreeDelete(terrain->root);

	free(terrain);
}
