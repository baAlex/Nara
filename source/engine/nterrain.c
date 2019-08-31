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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static inline float sDimension(const struct NTerrainNode* node)
{
	return (node->max.x - node->min.x);
}

static inline struct Vector2 sMiddle(const struct NTerrainNode* node)
{
	return (struct Vector2){node->min.x + sDimension(node) / 2.0, node->min.y + sDimension(node) / 2.0};
}


/*-----------------------------

 sGenerateIndex()
-----------------------------*/
static void sGenerateIndex(uint16_t* index_out, struct Vector2 min, struct Vector2 max, float pattern_dimension,
                           struct Vector2 org_min, struct Vector2 org_max, float org_pattern_dimension)
{
	// I promise to clean what follows...
	// All the float conversions are evil, evil! D:

	size_t squares = (size_t)ceil((max.x - min.x) / pattern_dimension);
	size_t length = squares * squares * 6;
	size_t col = 0;

	size_t org_squares = (size_t)ceil((org_max.x - org_min.x) / org_pattern_dimension);
	size_t org_col_start = (size_t)ceil((min.x - org_min.x) / org_pattern_dimension);
	size_t org_row = (size_t)ceil((min.y - org_min.y) / org_pattern_dimension);
	size_t org_col = org_col_start;

	size_t org_step = (org_squares / squares) / round((org_max.x - org_min.x) / (max.x - min.x));

	/*printf("Node sqrs: %zu, Org sqrs: %zu, Node sz: %zu, Org sz: %zu -> Step: %zu (sz diff: %f)\n", squares,
	       org_squares, (size_t)ceil((max.x - min.x)), (size_t)ceil((org_max.x - org_min.x)), org_step,
	       round((org_max.x - org_min.x) / (max.x - min.x)));*/

	for (size_t i = 0; i < length; i += 6)
	{
		#if 1
		index_out[i + 0] = org_col + (org_squares + 1) * org_row;
		index_out[i + 1] = org_col + (org_squares + 1) * org_row + org_step;
		index_out[i + 2] = org_col + (org_squares + 1) * org_row + org_step + ((org_squares + 1) * org_step);
		#else
		index_out[i + 0] = 0;
		index_out[i + 1] = 0;
		index_out[i + 2] = 0;
		#endif

		#if 1
		index_out[i + 3] = org_col + ((org_squares + 1) * org_row) + org_step + (org_squares + 1) * org_step;
		index_out[i + 4] = org_col + ((org_squares + 1) * org_row) + (org_squares + 1) * org_step;
		index_out[i + 5] = org_col + ((org_squares + 1) * org_row);
		#else
		index_out[i + 3] = 0;
		index_out[i + 4] = 0;
		index_out[i + 5] = 0;
		#endif

		// Next step
		col += 1;
		org_col += org_step;

		if ((col % squares) == 0)
		{
			col = 0;
			org_col = org_col_start;
			org_row += org_step;
		}
	}
}


/*-----------------------------

 sGenerateVertices()
-----------------------------*/
static void sGenerateVertices(struct Vertex* vertices_out, struct Vector2 min, struct Vector2 max,
                              float pattern_dimension, float terrain_dimension)
{
	int patterns_no = (max.x - min.x) / pattern_dimension;
	float texture_step_x = min.x / terrain_dimension;
	float texture_step_y = min.y / terrain_dimension;

	for (int row = 0; row < (patterns_no + 1); row++)
	{
		for (int col = 0; col < (patterns_no + 1); col++)
		{
			vertices_out[col + (patterns_no + 1) * row].uv.x = texture_step_x;
			vertices_out[col + (patterns_no + 1) * row].uv.y = texture_step_y;

			vertices_out[col + (patterns_no + 1) * row].pos.x = min.x + (float)(col * pattern_dimension);
			vertices_out[col + (patterns_no + 1) * row].pos.y = min.y + (float)(row * pattern_dimension);
			vertices_out[col + (patterns_no + 1) * row].pos.z = 0.0;

			texture_step_x += (float)pattern_dimension / terrain_dimension;
		}

		texture_step_y += (float)pattern_dimension / terrain_dimension;
		texture_step_x = 0.0;
	}
}


/*-----------------------------

 sAllocateIndex()
-----------------------------*/
static uint16_t* sAllocateIndex(struct NTerrainNode* node)
{
	node->index_no = (size_t)ceil(sDimension(node) / node->pattern_dimension);
	node->index_no = node->index_no * node->index_no * 6;
	return node->index = malloc(node->index_no * sizeof(uint16_t));
}


/*-----------------------------

 sAllocateVertices()
-----------------------------*/
static struct Vertex* sAllocateVertices(struct NTerrainNode* node, float pattern_dimension)
{
	node->vertices_no = (size_t)ceil(sDimension(node) / pattern_dimension + 1.0);
	node->vertices_no = node->vertices_no * node->vertices_no;
	return node->vertices = malloc(sizeof(struct Vertex) * node->vertices_no);
}


/*-----------------------------

 sSubdivideTile()
-----------------------------*/
static void sSubdivideTile(struct Tree* item, float min_tile_dimension)
{
	struct NTerrainNode* node = item->data;
	struct NTerrainNode* new_node = NULL;
	struct Tree* new_item = NULL;

	// TODO: With all the loss of precision, can the equal comparision fail? (I think that yes? no?)
	if ((sDimension(node) / 3.0) <= min_tile_dimension)
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
			new_node->max.x = node->min.x + sDimension(node) / 3.0;
			new_node->max.y = node->min.y + sDimension(node) / 3.0;
			break;
		case 1: // North
			new_node->min.x = node->min.x + sDimension(node) / 3.0;
			new_node->min.y = node->min.y;
			new_node->max.x = node->min.x + sDimension(node) / 3.0 * 2.0;
			new_node->max.y = node->min.y + sDimension(node) / 3.0;
			break;
		case 2: // North East
			new_node->min.x = node->min.x + sDimension(node) / 3.0 * 2.0;
			new_node->min.y = node->min.y;
			new_node->max.x = node->min.x + sDimension(node) / 3.0 * 3.0;
			new_node->max.y = node->min.y + sDimension(node) / 3.0;
			break;
		case 3: // West
			new_node->min.x = node->min.x;
			new_node->min.y = node->min.y + sDimension(node) / 3.0;
			new_node->max.x = node->min.x + sDimension(node) / 3.0;
			new_node->max.y = node->min.y + sDimension(node) / 3.0 * 2.0;
			break;
		case 4: // Center
			new_node->min.x = node->min.x + sDimension(node) / 3.0;
			new_node->min.y = node->min.y + sDimension(node) / 3.0;
			new_node->max.x = node->min.x + sDimension(node) / 3.0 * 2.0;
			new_node->max.y = node->min.y + sDimension(node) / 3.0 * 2.0;
			break;
		case 5: // East
			new_node->min.x = node->min.x + sDimension(node) / 3.0 * 2.0;
			new_node->min.y = node->min.y + sDimension(node) / 3.0;
			new_node->max.x = node->min.x + sDimension(node) / 3.0 * 3.0;
			new_node->max.y = node->min.y + sDimension(node) / 3.0 * 2.0;
			break;
		case 6: // South West
			new_node->min.x = node->min.x;
			new_node->min.y = node->min.y + sDimension(node) / 3.0 * 2.0;
			new_node->max.x = node->min.x + sDimension(node) / 3.0;
			new_node->max.y = node->min.y + sDimension(node) / 3.0 * 3.0;
			break;
		case 7: // South
			new_node->min.x = node->min.x + sDimension(node) / 3.0;
			new_node->min.y = node->min.y + sDimension(node) / 3.0 * 2.0;
			new_node->max.x = node->min.x + sDimension(node) / 3.0 * 2.0;
			new_node->max.y = node->min.y + sDimension(node) / 3.0 * 3.0;
			break;
		case 8: // South East
			new_node->min.x = node->min.x + sDimension(node) / 3.0 * 2.0;
			new_node->min.y = node->min.y + sDimension(node) / 3.0 * 2.0;
			new_node->max.x = node->min.x + sDimension(node) / 3.0 * 3.0;
			new_node->max.y = node->min.y + sDimension(node) / 3.0 * 3.0;
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

	for (float i = dimension; i >= min_tile_dimension; i /= 3.0)
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
	struct NTerrainNode* last_parent = NULL;
	size_t p_depth = 0;

	s.start = terrain->root;

	while ((item = TreeIterate(&s, &buffer)) != NULL)
	{
		node = item->data;
		terrain->tiles_no++;

		node->pattern_dimension = sDimension(node);

		for (int i = 0; i < pattern_subdivisions; i++)
			node->pattern_dimension /= 3.0;

		// Any parent has vertices to share?
		if (last_parent != NULL && s.depth > p_depth)
		{
			node->vertices_type = INHERITED_FROM_PARENT;
			node->vertices = NULL;

			sAllocateIndex(node);
			sGenerateIndex(node->index, node->min, node->max, node->pattern_dimension, last_parent->min,
			               last_parent->max, terrain->min_pattern_dimension);
		}
		else
		{
			// Can this node keep all childrens vertices? (because indices in OpenGL ES2)
			if (powf(sDimension(node) / terrain->min_pattern_dimension + 1, 2.0) < UINT16_MAX)
			{
				last_parent = node;
				p_depth = s.depth;
				node->vertices_type = SHARED_WITH_CHILDRENS;

				terrain->vertices_buffers_no++;

				sAllocateVertices(node, terrain->min_pattern_dimension);
				sGenerateVertices(node->vertices, node->min, node->max, terrain->min_pattern_dimension, terrain->dimension);

				sAllocateIndex(node);
				sGenerateIndex(node->index, node->min, node->max, node->pattern_dimension, node->min, node->max,
				               terrain->min_pattern_dimension);
			}

			// Nope, only his own vertices
			else
			{
				last_parent = NULL;
				node->vertices_type = OWN_VERTICES;

				terrain->vertices_buffers_no++;

				sAllocateVertices(node, node->pattern_dimension);
				sGenerateVertices(node->vertices, node->min, node->max, node->pattern_dimension, terrain->dimension);

				sAllocateIndex(node);
				sGenerateIndex(node->index, node->min, node->max, node->pattern_dimension, node->min, node->max,
				               node->pattern_dimension);
			}
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

			if (node->index != NULL)
				free(node->index);

			if (node->vertices != NULL)
				free(node->vertices);
		}

		TreeDelete(terrain->root);
	}

	free(terrain);
}


/*-----------------------------

 NTerrainIterate()
-----------------------------*/
struct NTerrainNode* NTerrainIterate(struct TreeState* state, struct Buffer* buffer, struct NTerrainNode** out_shared,
                                     struct Vector2 position)
{
	struct Tree** future_parent_heap = NULL;

	if (state->start != NULL)
	{
		state->future_parent[0] = NULL;
		state->future_depth = 1;
		state->future_return = state->start;
		state->actual = state->start;
		state->start = NULL;
	}

again:

	// Actual node (to call)
	if ((state->actual = state->future_return) == NULL)
		return NULL;

	state->depth = state->future_depth;
	state->future_return = NULL;

	if(((struct NTerrainNode*)state->actual->data)->vertices_type ==  SHARED_WITH_CHILDRENS)
		*out_shared = state->actual->data;

	// Future values
	{
		// Go in? (childrens)
		// TODO, the 1.5 can be configurable
		// TODO, calculate as an circle is too lazy
		if (state->actual->children != NULL &&
		    Vector2Distance(sMiddle(state->actual->data), position) <= sDimension(state->actual->data) * 1.5)
		{
			if (buffer->size < (state->depth + 1) * sizeof(void*))
			if (BufferResize(buffer, (state->depth + 1) * sizeof(void*)) == NULL) // FIXME, Bug in LibJapan, the +1
				return NULL;

			future_parent_heap = buffer->data;
			future_parent_heap[state->depth] = state->actual->next;
			state->future_return = state->actual->children;

			state->future_depth++;
			goto again; // We omit actual for being drawing
		}

		// Brothers
		else if (state->depth > 0 && state->actual->next != NULL)
		{
			state->future_return = state->actual->next;
		}

		// Go out (next parent)
		else if (state->depth > 0)
		{
			future_parent_heap = buffer->data;

			while ((state->future_depth -= 1) > 0)
			{
				if (future_parent_heap[state->future_depth] != NULL)
				{
					state->future_return = future_parent_heap[state->future_depth];
					break;
				}
			}
		}
	}

	return state->actual->data;
}


/*-----------------------------

 NTerrainShape()
-----------------------------*/
static inline float sPixelAsFloat(const struct Image* image, float x, float y)
{
	union
	{
		uint8_t u8;
		uint16_t u16;
	} pixel;

	x *= image->width;
	y *= image->height;

	switch (image->format)
	{
	case IMAGE_GRAY8:
		pixel.u8 = ((uint8_t*)image->data)[(int)floor(x) + image->width * (int)floor(y)];
		return (float)pixel.u8 / 255.0;
	case IMAGE_GRAY16:
		pixel.u16 = ((uint16_t*)image->data)[(int)floor(x) + image->width * (int)floor(y)];
		return (float)pixel.u16 / 65535.0;
	default:
		break;
	}

	return 0.0;
}

void NTerrainShape(struct NTerrain* terrain, float elevation, const char* heightmap_filename)
{
	struct Tree* item = NULL;
	struct Buffer buffer = {0};
	struct TreeState s = {0};
	struct NTerrainNode* node = NULL;

	struct Status st = {0};
	struct Image* image = NULL;

	if((image = ImageLoad(heightmap_filename, &st)) == NULL)
	{
		StatusPrint("NTerrainShape", st);
		return;
	}

	s.start = terrain->root;

	while ((item = TreeIterate(&s, &buffer)) != NULL)
	{
		node = item->data;

		if(node->vertices_type == INHERITED_FROM_PARENT)
			continue;

		for(size_t i = 0; i < node->vertices_no; i++)
		{
			node->vertices[i].pos.z = sPixelAsFloat(image, node->vertices[i].uv.x, node->vertices[i].uv.y);
			node->vertices[i].pos.z *= elevation;
		}
	}
}
