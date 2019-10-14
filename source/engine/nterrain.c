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

// TODO: while having differenced max, min Z values for every node sounds
// as a good idea, it actually made the prediction on where the LOD
// quality changes a lot more difficult (now is necessary to take in account
// the elevation on that precise node). And fill gaps really need to
// anticipate where the quality changes. Having a "fake" z values shared
// across all the terrain is going to be a lot better.


static inline float sDimension(const struct NTerrainNode* node)
{
	return (node->max.x - node->min.x);
}

static inline size_t sSquareColumns(float dimension, float pattern_dimension)
{
	return (size_t)(ceilf(dimension / pattern_dimension));
}

static inline struct Vector3 sMiddle(const struct NTerrainNode* node)
{
	return (struct Vector3){
	    .x = node->min.x + (node->max.x - node->min.x) / 2.0f,
	    .y = node->min.y + (node->max.y - node->min.y) / 2.0f,
	    .z = node->min.z + (node->max.z - node->min.z) / 2.0f,
	};
}

static inline bool sBoxSphereCollition(struct Vector3 sphere_origin, float sphere_radious, struct Vector3 box_min,
                                       struct Vector3 box_max)
{
	// TODO: I think that a box (rather than an sphere) is going to be better
	if (sphere_origin.x > (box_min.x - sphere_radious) && sphere_origin.x < (box_max.x + sphere_radious) &&
	    sphere_origin.y > (box_min.y - sphere_radious) && sphere_origin.y < (box_max.y + sphere_radious) &&
	    sphere_origin.z > (box_min.z - sphere_radious) && sphere_origin.z < (box_max.z + sphere_radious))
		return true;

	return false;
}


/*-----------------------------

 sPixelAsFloat()
-----------------------------*/
float sPixelAsFloat(const struct Image* image, struct Vector2 cords)
{
	// https://en.wikipedia.org/wiki/Bilinear_filtering
	union {
		uint8_t* u8;
		uint16_t* u16;
		void* raw;

	} data = {.raw = image->data};

	if (cords.x > 1.0f)
		cords.x = 1.0f;
	if (cords.y > 1.0f)
		cords.y = 1.0f;
	if (cords.x < -1.0f)
		cords.x = -1.0f;
	if (cords.y < -1.0f)
		cords.y = -1.0f;

	// TODO, think a better replacement for the -1
	float u = cords.x * (float)(image->width - 1);
	float v = cords.y * (float)(image->height - 1);

	size_t x = (size_t)floorf(u);
	size_t y = (size_t)floorf(v);

	float u_ratio = u - floorf(u);
	float v_ratio = v - floorf(v);
	float u_opposite = 1.0f - u_ratio;
	float v_opposite = 1.0f - v_ratio;

	float result = 0.0;

	switch (image->format)
	{
	case IMAGE_GRAY8:
		result =
		    (data.u8[x + image->width * y] * u_opposite + data.u8[x + 1 + image->width * y] * u_ratio) * v_opposite +
		    (data.u8[x + image->width * (y + 1)] * u_opposite + data.u8[x + 1 + image->width * (y + 1)] * u_ratio) *
		        v_ratio;
		result /= 255.0f;
		break;

	case IMAGE_GRAY16:
		result =
		    (data.u16[x + image->width * y] * u_opposite + data.u16[x + 1 + image->width * y] * u_ratio) * v_opposite +
		    (data.u16[x + image->width * (y + 1)] * u_opposite + data.u16[x + 1 + image->width * (y + 1)] * u_ratio) *
		        v_ratio;
		result /= 65535.0f;
		break;

	default: break;
	}

	return result;
}


/*-----------------------------

 sGenerateIndex()
-----------------------------*/
static struct Index sGenerateIndex(struct Buffer* buffer, const struct NTerrainNode* node, struct Vector3 org_min,
                                   struct Vector3 org_max, float org_pattern_dimension)
{
	// Generation on a temporary buffer
	size_t const squares = sSquareColumns(sDimension(node), node->pattern_dimension);
	size_t const length = squares * squares * 6; // 6 = Two triangles vertices, OpenGL ES2 did not have quads
	size_t col = 0;

	BufferResize(buffer, sizeof(uint16_t) * length); // TODO, check failure
	uint16_t* temp_data = buffer->data;

	{
		size_t const org_squares = sSquareColumns(org_max.x - org_min.x, org_pattern_dimension);
		size_t const org_step = (org_squares / squares) / (size_t)lroundf((org_max.x - org_min.x) / sDimension(node));

		size_t const org_col_start = sSquareColumns(node->min.x - org_min.x, org_pattern_dimension);
		size_t org_row = sSquareColumns(node->min.y - org_min.y, org_pattern_dimension);
		size_t org_col = org_col_start;

		size_t index_value = 0;

		/*printf("Node sqrs: %zu, Org sqrs: %zu, Node sz: %zu, Org sz: %zu -> Step: %zu (sz diff: %f)\n", squares,
		       org_squares, (size_t)ceil((max.x - min.x)), (size_t)ceil((org_max.x - org_min.x)), org_step,
		       round((org_max.x - org_min.x) / (max.x - min.x)));*/

		for (size_t i = 0; i < length; i += 6)
		{
			// As next uint16_t conversions silently hide everything, this gently
			// comparasion would tell us about bad calculations, TODO: use only in tests.
			if (org_col + (org_squares + 1) * org_row + org_step + ((org_squares + 1) * org_step) > UINT16_MAX)
			{
				printf("Beep boop!!!\n"); // TODO
				return (struct Index){0};
			}

			index_value = org_col + (org_squares + 1) * org_row;
			temp_data[i + 0] = (uint16_t)index_value;

			index_value = org_col + (org_squares + 1) * org_row + org_step;
			temp_data[i + 1] = (uint16_t)index_value;

			index_value = org_col + (org_squares + 1) * org_row + org_step + ((org_squares + 1) * org_step);
			temp_data[i + 2] = (uint16_t)index_value;

			index_value = org_col + ((org_squares + 1) * org_row) + org_step + (org_squares + 1) * org_step;
			temp_data[i + 3] = (uint16_t)index_value;

			index_value = org_col + ((org_squares + 1) * org_row) + (org_squares + 1) * org_step;
			temp_data[i + 4] = (uint16_t)index_value;

			index_value = org_col + ((org_squares + 1) * org_row);
			temp_data[i + 5] = (uint16_t)index_value;

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

	// Copy to the final index
	struct Index index = {0};
	IndexInit(&index, temp_data, length, NULL); // TODO: Check error

	return index;
}


/*-----------------------------

 sGenerateVertices()
-----------------------------*/
static struct Vertices sGenerateVertices(struct Buffer* buffer, const struct NTerrainNode* node,
                                         float pattern_dimension, float terrain_dimension,
                                         const struct Image* heightmap, float elevation, float* out_min_z,
                                         float* out_max_z)
{
	float min_z = 0.0f;
	float max_z = 0.0f;

	// Generation on a temporary buffer
	size_t const patterns_no = sSquareColumns(sDimension(node), pattern_dimension);

	BufferResize(buffer, sizeof(struct Vertex) * (patterns_no + 1) * (patterns_no + 1));
	struct Vertex* temp_data = buffer->data;

	{
		struct Vector2 texture_step = {node->min.x / terrain_dimension, node->min.y / terrain_dimension};

		for (size_t row = 0; row < (patterns_no + 1); row++)
		{
			for (size_t col = 0; col < (patterns_no + 1); col++)
			{
				temp_data[col + (patterns_no + 1) * row].uv.x = texture_step.x;
				temp_data[col + (patterns_no + 1) * row].uv.y = texture_step.y;

				temp_data[col + (patterns_no + 1) * row].pos.x = node->min.x + (float)col * pattern_dimension;
				temp_data[col + (patterns_no + 1) * row].pos.y = node->min.y + (float)row * pattern_dimension;

				if (heightmap != NULL)
				{
					temp_data[col + (patterns_no + 1) * row].pos.z = sPixelAsFloat(heightmap, texture_step) * elevation;

					if (temp_data[col + (patterns_no + 1) * row].pos.z > max_z)
						max_z = temp_data[col + (patterns_no + 1) * row].pos.z;

					if (temp_data[col + (patterns_no + 1) * row].pos.z < min_z)
						min_z = temp_data[col + (patterns_no + 1) * row].pos.z;
				}
				else
					temp_data[col + (patterns_no + 1) * row].pos.z = 0.0;

				// Next step
				texture_step.x += pattern_dimension / terrain_dimension;
			}

			texture_step.y += pattern_dimension / terrain_dimension;
			texture_step.x = node->min.x / terrain_dimension;
		}
	}

	if (out_min_z != NULL && out_max_z != NULL)
	{
		*out_min_z = min_z;
		*out_max_z = max_z;
	}

	// Copy to the final index
	struct Vertices vertices = {0};
	VerticesInit(&vertices, temp_data, (patterns_no + 1) * (patterns_no + 1), NULL); // TODO: Check error

	return vertices;
}


/*-----------------------------

 sSubdivideTile()
-----------------------------*/
static void sSubdivideTile(struct Tree* item, float min_tile_dimension)
{
	struct NTerrainNode* node = item->data;
	struct NTerrainNode* new_node = NULL;
	struct Tree* new_item = NULL;

	if ((sDimension(node) / 3.0f) < min_tile_dimension)
		return;

	for (int i = 0; i < 9; i++)
	{
		new_item = TreeCreate(item, NULL, sizeof(struct NTerrainNode));
		new_node = new_item->data;

		new_node->min.z = 0.0; // Is impossible to do in this step, as this function
		new_node->max.z = 0.0; // did not take the heightmap in account

		switch (i)
		{
		case 0: // North West
			new_node->min.x = node->min.x;
			new_node->min.y = node->min.y;
			new_node->max.x = node->min.x + sDimension(node) / 3.0f;
			new_node->max.y = node->min.y + sDimension(node) / 3.0f;
			break;
		case 1: // North
			new_node->min.x = node->min.x + sDimension(node) / 3.0f;
			new_node->min.y = node->min.y;
			new_node->max.x = node->min.x + sDimension(node) / 3.0f * 2.0f;
			new_node->max.y = node->min.y + sDimension(node) / 3.0f;
			break;
		case 2: // North East
			new_node->min.x = node->min.x + sDimension(node) / 3.0f * 2.0f;
			new_node->min.y = node->min.y;
			new_node->max.x = node->min.x + sDimension(node) / 3.0f * 3.0f;
			new_node->max.y = node->min.y + sDimension(node) / 3.0f;
			break;
		case 3: // West
			new_node->min.x = node->min.x;
			new_node->min.y = node->min.y + sDimension(node) / 3.0f;
			new_node->max.x = node->min.x + sDimension(node) / 3.0f;
			new_node->max.y = node->min.y + sDimension(node) / 3.0f * 2.0f;
			break;
		case 4: // Center
			new_node->min.x = node->min.x + sDimension(node) / 3.0f;
			new_node->min.y = node->min.y + sDimension(node) / 3.0f;
			new_node->max.x = node->min.x + sDimension(node) / 3.0f * 2.0f;
			new_node->max.y = node->min.y + sDimension(node) / 3.0f * 2.0f;
			break;
		case 5: // East
			new_node->min.x = node->min.x + sDimension(node) / 3.0f * 2.0f;
			new_node->min.y = node->min.y + sDimension(node) / 3.0f;
			new_node->max.x = node->min.x + sDimension(node) / 3.0f * 3.0f;
			new_node->max.y = node->min.y + sDimension(node) / 3.0f * 2.0f;
			break;
		case 6: // South West
			new_node->min.x = node->min.x;
			new_node->min.y = node->min.y + sDimension(node) / 3.0f * 2.0f;
			new_node->max.x = node->min.x + sDimension(node) / 3.0f;
			new_node->max.y = node->min.y + sDimension(node) / 3.0f * 3.0f;
			break;
		case 7: // South
			new_node->min.x = node->min.x + sDimension(node) / 3.0f;
			new_node->min.y = node->min.y + sDimension(node) / 3.0f * 2.0f;
			new_node->max.x = node->min.x + sDimension(node) / 3.0f * 2.0f;
			new_node->max.y = node->min.y + sDimension(node) / 3.0f * 3.0f;
			break;
		case 8: // South East
			new_node->min.x = node->min.x + sDimension(node) / 3.0f * 2.0f;
			new_node->min.y = node->min.y + sDimension(node) / 3.0f * 2.0f;
			new_node->max.x = node->min.x + sDimension(node) / 3.0f * 3.0f;
			new_node->max.y = node->min.y + sDimension(node) / 3.0f * 3.0f;
		}

		sSubdivideTile(new_item, min_tile_dimension);
	}
}


/*-----------------------------

 NTerrainCreate()
-----------------------------*/
struct NTerrain* NTerrainCreate(const char* heightmap_filename, float elevation, float dimension,
                                float min_tile_dimension, int pattern_subdivisions, struct Status* st)
{
	struct NTerrain* terrain = NULL;
	struct NTerrainNode* node = NULL;

	struct Tree* item = NULL;
	struct Image* heightmap = NULL;

	struct TreeState state = {0};
	struct NTerrainNode* last_parent = NULL;
	size_t parent_depth = 0;

	StatusSet(st, "NTerraiNCreate", STATUS_SUCCESS, NULL);

	if (heightmap_filename != NULL)
	{
		if ((heightmap = ImageLoad(heightmap_filename, st)) == NULL)
			goto return_failure;
	}

	if ((terrain = calloc(1, sizeof(struct NTerrain))) == NULL ||
	    (terrain->root = TreeCreate(NULL, NULL, sizeof(struct NTerrainNode))) == NULL)
	{
		StatusSet(st, "NTerraiNCreate", STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	terrain->dimension = dimension;

	// Mesures
	for (float i = dimension; i >= min_tile_dimension; i /= 3.0f)
	{
		terrain->min_tile_dimension = i;
		terrain->min_pattern_dimension = i;
		terrain->steps++;
	}

	for (int i = 0; i < pattern_subdivisions; i++)
		terrain->min_pattern_dimension /= 3.0f;

	// Tile subdivision
	node = terrain->root->data;
	node->min = (struct Vector3){0.0, 0.0, 0.0};
	node->max = (struct Vector3){dimension, dimension, 0.0};

	sSubdivideTile(terrain->root, min_tile_dimension);

	// Pattern subdivision (the following big loop)
	// generating vertices and indexes
	state.start = terrain->root;

	while ((item = TreeIterate(&state, &terrain->buffer)) != NULL)
	{
		node = item->data;
		terrain->tiles_no++;

		node->pattern_dimension = sDimension(node);

		for (int i = 0; i < pattern_subdivisions; i++)
			node->pattern_dimension /= 3.0f;

		// Any parent has vertices to share?
		if (last_parent != NULL && state.depth > parent_depth)
		{
			node->vertices_type = INHERITED_FROM_PARENT;
			node->index = sGenerateIndex(&terrain->buffer, node, last_parent->min, last_parent->max,
			                             terrain->min_pattern_dimension);

			node->min.z = last_parent->min.z;
			node->max.z = last_parent->max.z;
		}
		else
		{
			// Can this node keep all childrens vertices? (because indices in OpenGL ES2)
			if (ceilf(powf(sDimension(node) / terrain->min_pattern_dimension + 1.0f, 2.0)) < (float)UINT16_MAX)
			{
				last_parent = node;
				parent_depth = state.depth;
				terrain->vertices_buffers_no++;

				node->vertices_type = SHARED_WITH_CHILDRENS;
				node->vertices =
				    sGenerateVertices(&terrain->buffer, node, terrain->min_pattern_dimension, terrain->dimension,
				                      heightmap, elevation, &node->min.z, &node->max.z);
				node->index =
				    sGenerateIndex(&terrain->buffer, node, node->min, node->max, terrain->min_pattern_dimension);
			}

			// Nope, only his own vertices
			else
			{
				last_parent = NULL;
				parent_depth = 0;
				terrain->vertices_buffers_no++;

				node->vertices_type = OWN_VERTICES;
				node->vertices = sGenerateVertices(&terrain->buffer, node, node->pattern_dimension, terrain->dimension,
				                                   heightmap, elevation, &node->min.z, &node->max.z);
				node->index = sGenerateIndex(&terrain->buffer, node, node->min, node->max, node->pattern_dimension);
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
	struct Tree* item = NULL;
	struct NTerrainNode* node = NULL;

	if (terrain->root != NULL)
	{
		while ((item = TreeIterate(&s, &terrain->buffer)) != NULL)
		{
			node = item->data;

			IndexFree(&node->index);
			VerticesFree(&node->vertices);
		}

		TreeDelete(terrain->root);
	}

	BufferClean(&terrain->buffer);
	free(terrain);
}


/*-----------------------------

 NTerrainIterate()
-----------------------------*/
struct NTerrainNode* NTerrainIterate(struct TreeState* state, struct Buffer* buffer,
                                     struct NTerrainNode** out_with_vertices, struct Vector3 camera_position)
{
	struct Tree** future_parent_heap = NULL;
	struct NTerrainNode* actual_node = NULL;

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
	state->future_return = NULL; // This one is resolved in next conditionals

	actual_node = state->actual->data;

	if (actual_node->vertices_type == SHARED_WITH_CHILDRENS || actual_node->vertices_type == OWN_VERTICES)
		*out_with_vertices = actual_node;

	// Future values
	{
		// Go in? (childrens)
		float distance_factor = sDimension(actual_node);
		// float distance_factor = sDimension(actual_node) * 4.0f; // Increased distance quality (configurable)

		if (state->actual->children != NULL &&
		    sBoxSphereCollition(camera_position, distance_factor, actual_node->min, actual_node->max) == true)
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

	return actual_node;
}


/*-----------------------------

 NTerrainDraw()
-----------------------------*/
int NTerrainDraw(struct NTerrain* terrain, float max_distance, struct Vector3 camera_position)
{
	struct TreeState s = {.start = terrain->root};

	struct NTerrainNode* node = NULL;
	struct NTerrainNode* last_with_vertices = NULL;
	struct NTerrainNode* temp = NULL;

	int dcalls = 0;

	while ((node = NTerrainIterate(&s, &terrain->buffer, &last_with_vertices, camera_position)) != NULL)
	{
		if(sBoxSphereCollition(camera_position, max_distance, node->min, node->max) == false)
			continue;

		if (temp != last_with_vertices)
		{
			temp = last_with_vertices;

#ifndef TEST
			glBindBuffer(GL_ARRAY_BUFFER, last_with_vertices->vertices.glptr);
			glVertexAttribPointer(ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), NULL);
			glVertexAttribPointer(ATTRIBUTE_UV, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), (float*)NULL + 3);
			dcalls += 3;
#endif
		}

#ifndef TEST
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, node->index.glptr);
		glDrawElements(GL_TRIANGLES, (GLsizei)node->index.length, GL_UNSIGNED_SHORT, NULL);
		dcalls += 2;
#endif
	}

	return dcalls;
}


/*-----------------------------

 NTerrainPrintInfo()
-----------------------------*/
void NTerrainPrintInfo(const struct NTerrain* terrain)
{
	struct TreeState s = {.start = terrain->root};
	struct Buffer buffer = {0};
	struct Tree* item = NULL;
	struct NTerrainNode* node = NULL;

	size_t prev_depth = 0;

	printf("Terrain 0x%p:\n", (void*)terrain);
	printf(" - Dimension: %f\n", terrain->dimension);
	printf(" - Minimum tile dimension: %f\n", terrain->min_tile_dimension);
	printf(" - Minimum pattern dimension: %f\n", terrain->min_pattern_dimension);
	printf(" - Steps: %lu\n", terrain->steps);
	printf(" - Tiles: %lu\n", terrain->tiles_no);
	printf(" - Vertices buffers: %lu\n", terrain->vertices_buffers_no);

	while ((item = TreeIterate(&s, &buffer)) != NULL)
	{
		node = item->data;

		if (prev_depth < s.depth || s.depth == 0)
		{
			printf(" # Step %lu:\n", s.depth);
			printf("    - Dimension: %f\n", (node->max.x - node->min.x));
			printf("    - Pattern dimension: %f\n", node->pattern_dimension);
			printf("    - Vertices type: %i\n", node->vertices_type);

			prev_depth = s.depth;
		}
	}
}
