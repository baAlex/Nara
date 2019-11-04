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
#include "utilities.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static inline bool sRectRectCollition(struct Vector2 a_min, struct Vector2 a_max, struct Vector2 b_min,
                                      struct Vector2 b_max)
{
	if (a_max.x < b_min.x || a_max.y < b_min.y || a_min.x > b_max.x || a_min.y > b_max.y)
		return false;

	return true;
}

static inline bool sCircleRectCollition(struct Vector2 circle_origin, float circle_radius, struct Vector2 rect_,
                                        struct Vector2 box_max)
{
	if (circle_origin.x < (rect_.x - circle_radius) || circle_origin.x > (box_max.x + circle_radius) ||
	    circle_origin.y < (rect_.y - circle_radius) || circle_origin.y > (box_max.y + circle_radius))
		return false;

	return true;
}

static inline bool sSphereBoxCollition(struct Vector3 sphere_origin, float sphere_radius, struct Vector3 box_min,
                                       struct Vector3 box_max)
{
	if (sphere_origin.x < (box_min.x - sphere_radius) || sphere_origin.x > (box_max.x + sphere_radius) ||
	    sphere_origin.y < (box_min.y - sphere_radius) || sphere_origin.y > (box_max.y + sphere_radius) ||
	    sphere_origin.z < (box_min.z - sphere_radius) || sphere_origin.z > (box_max.z + sphere_radius))
		return false;

	return true;
}

static inline float sNodeDimension(const struct NTerrainNode* node)
{
	return (node->max.x - node->min.x);
}

static inline struct Vector2 sNodeMiddle(const struct NTerrainNode* node)
{
	return (struct Vector2){.x = node->min.x + (node->max.x - node->min.x) / 2.0f,
	                        .y = node->min.y + (node->max.y - node->min.y) / 2.0f};
}

static inline size_t sPatternsInARow(float dimension, float pattern_dimension)
{
	return (size_t)(ceilf(dimension / pattern_dimension));
}


/*-----------------------------

 sNodeCreate()
-----------------------------*/
static struct NTerrainNode* sNodeCreate(struct NTerrainNode* parent)
{
	struct NTerrainNode* node = NULL;

	if ((node = calloc(1, sizeof(struct NTerrainNode))) != NULL)
	{
		node->children = NULL;
		node->next = NULL;
		node->parent = parent;

		if (parent != NULL)
		{
			if (parent->children == NULL)
				node->next = NULL;
			else
				node->next = parent->children;

			parent->children = node;
		}
	}

	return node;
}


/*-----------------------------

 sHeightmapElevation()
-----------------------------*/
static float sHeightmapElevation(const struct Image* hmap, struct Vector2 cords)
{
	// https://en.wikipedia.org/wiki/Bilinear_filtering
	union {
		uint8_t* u8;
		uint16_t* u16;
		void* raw;

	} data = {.raw = hmap->data};

	if (cords.x > 1.0f)
		cords.x = 1.0f;
	if (cords.y > 1.0f)
		cords.y = 1.0f;
	if (cords.x < -1.0f)
		cords.x = -1.0f;
	if (cords.y < -1.0f)
		cords.y = -1.0f;

	// TODO, think a better replacement for the -1
	float u = cords.x * (float)(hmap->width - 1);
	float v = cords.y * (float)(hmap->height - 1);

	size_t x = (size_t)floorf(u);
	size_t y = (size_t)floorf(v);

	float u_ratio = u - floorf(u);
	float v_ratio = v - floorf(v);
	float u_opposite = 1.0f - u_ratio;
	float v_opposite = 1.0f - v_ratio;

	float result = 0.0;

	switch (hmap->format)
	{
	case IMAGE_GRAY8:
		result = (data.u8[x + hmap->width * y] * u_opposite + data.u8[x + 1 + hmap->width * y] * u_ratio) * v_opposite +
		         (data.u8[x + hmap->width * (y + 1)] * u_opposite + data.u8[x + 1 + hmap->width * (y + 1)] * u_ratio) *
		             v_ratio;
		result /= 255.0f;
		break;

	case IMAGE_GRAY16:
		result =
		    (data.u16[x + hmap->width * y] * u_opposite + data.u16[x + 1 + hmap->width * y] * u_ratio) * v_opposite +
		    (data.u16[x + hmap->width * (y + 1)] * u_opposite + data.u16[x + 1 + hmap->width * (y + 1)] * u_ratio) *
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
	size_t const patterns_no = sPatternsInARow(sNodeDimension(node), node->pattern_dimension);
	size_t const length = patterns_no * patterns_no * 6; // 6 = Two triangles vertices, as OpenGL ES2 did not have quads
	size_t col = 0;

	BufferResize(buffer, sizeof(uint16_t) * length); // TODO, check failure
	uint16_t* temp_data = buffer->data;

	{
		size_t const org_patterns_no = sPatternsInARow(org_max.x - org_min.x, org_pattern_dimension);
		size_t const org_step =
		    (org_patterns_no / patterns_no) / (size_t)lroundf((org_max.x - org_min.x) / sNodeDimension(node));

		size_t const org_col_start = sPatternsInARow(node->min.x - org_min.x, org_pattern_dimension);
		size_t org_row = sPatternsInARow(node->min.y - org_min.y, org_pattern_dimension);
		size_t org_col = org_col_start;

		size_t index_value = 0;

		/*printf("Node sqrs: %zu, Org sqrs: %zu, Node sz: %zu, Org sz: %zu -> Step: %zu (sz diff: %f)\n", patterns_no,
		       org_patterns_no, (size_t)ceil((max.x - min.x)), (size_t)ceil((org_max.x - org_min.x)), org_step,
		       round((org_max.x - org_min.x) / (max.x - min.x)));*/

		for (size_t i = 0; i < length; i += 6)
		{
			// As next uint16_t conversions silently hide everything, this gently
			// comparasion would tell us about bad calculations, TODO: use only in tests.
			if (org_col + (org_patterns_no + 1) * org_row + org_step + ((org_patterns_no + 1) * org_step) > UINT16_MAX)
			{
				printf("Beep boop!!!\n"); // TODO
				return (struct Index){0};
			}

			index_value = org_col + (org_patterns_no + 1) * org_row;
			temp_data[i + 0] = (uint16_t)index_value;

			index_value = org_col + (org_patterns_no + 1) * org_row + org_step;
			temp_data[i + 1] = (uint16_t)index_value;

			index_value = org_col + (org_patterns_no + 1) * org_row + org_step + ((org_patterns_no + 1) * org_step);
			temp_data[i + 2] = (uint16_t)index_value;

			index_value = org_col + ((org_patterns_no + 1) * org_row) + org_step + (org_patterns_no + 1) * org_step;
			temp_data[i + 3] = (uint16_t)index_value;

			index_value = org_col + ((org_patterns_no + 1) * org_row) + (org_patterns_no + 1) * org_step;
			temp_data[i + 4] = (uint16_t)index_value;

			index_value = org_col + ((org_patterns_no + 1) * org_row);
			temp_data[i + 5] = (uint16_t)index_value;

			// Next step
			col += 1;
			org_col += org_step;

			if ((col % patterns_no) == 0)
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
                                         float pattern_dimension, float terrain_dimension, const struct Image* hmap,
                                         float elevation)
{
	// Generation on a temporary buffer
	size_t const patterns_no = sPatternsInARow(sNodeDimension(node), pattern_dimension);

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

				if (hmap != NULL)
					temp_data[col + (patterns_no + 1) * row].pos.z =
					    sHeightmapElevation(hmap, texture_step) * elevation;
				else
					temp_data[col + (patterns_no + 1) * row].pos.z = 0.0;

				// Next step
				texture_step.x += pattern_dimension / terrain_dimension;
			}

			texture_step.y += pattern_dimension / terrain_dimension;
			texture_step.x = node->min.x / terrain_dimension;
		}
	}

	// Copy to the final index
	struct Vertices vertices = {0};
	VerticesInit(&vertices, temp_data, (patterns_no + 1) * (patterns_no + 1), NULL); // TODO: Check error

	return vertices;
}


/*-----------------------------

 sSubdivideNode()
-----------------------------*/
static void sSubdivideNode(struct NTerrainNode* node, float min_node_dimension)
{
	struct NTerrainNode* new_node = NULL;

	if ((sNodeDimension(node) / 3.0f) < min_node_dimension)
		return;

	for (int i = 0; i < 9; i++)
	{
		new_node = sNodeCreate(node);

		new_node->min.z = node->min.z;
		new_node->max.z = node->max.z;

		switch (i)
		{
		case 0: // North West
			new_node->min.x = node->min.x;
			new_node->min.y = node->min.y;
			new_node->max.x = node->min.x + sNodeDimension(node) / 3.0f;
			new_node->max.y = node->min.y + sNodeDimension(node) / 3.0f;
			break;
		case 1: // North
			new_node->min.x = node->min.x + sNodeDimension(node) / 3.0f;
			new_node->min.y = node->min.y;
			new_node->max.x = node->min.x + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->max.y = node->min.y + sNodeDimension(node) / 3.0f;
			break;
		case 2: // North East
			new_node->min.x = node->min.x + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->min.y = node->min.y;
			new_node->max.x = node->min.x + sNodeDimension(node) / 3.0f * 3.0f;
			new_node->max.y = node->min.y + sNodeDimension(node) / 3.0f;
			break;
		case 3: // West
			new_node->min.x = node->min.x;
			new_node->min.y = node->min.y + sNodeDimension(node) / 3.0f;
			new_node->max.x = node->min.x + sNodeDimension(node) / 3.0f;
			new_node->max.y = node->min.y + sNodeDimension(node) / 3.0f * 2.0f;
			break;
		case 4: // Center
			new_node->min.x = node->min.x + sNodeDimension(node) / 3.0f;
			new_node->min.y = node->min.y + sNodeDimension(node) / 3.0f;
			new_node->max.x = node->min.x + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->max.y = node->min.y + sNodeDimension(node) / 3.0f * 2.0f;
			break;
		case 5: // East
			new_node->min.x = node->min.x + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->min.y = node->min.y + sNodeDimension(node) / 3.0f;
			new_node->max.x = node->min.x + sNodeDimension(node) / 3.0f * 3.0f;
			new_node->max.y = node->min.y + sNodeDimension(node) / 3.0f * 2.0f;
			break;
		case 6: // South West
			new_node->min.x = node->min.x;
			new_node->min.y = node->min.y + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->max.x = node->min.x + sNodeDimension(node) / 3.0f;
			new_node->max.y = node->min.y + sNodeDimension(node) / 3.0f * 3.0f;
			break;
		case 7: // South
			new_node->min.x = node->min.x + sNodeDimension(node) / 3.0f;
			new_node->min.y = node->min.y + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->max.x = node->min.x + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->max.y = node->min.y + sNodeDimension(node) / 3.0f * 3.0f;
			break;
		case 8: // South East
			new_node->min.x = node->min.x + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->min.y = node->min.y + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->max.x = node->min.x + sNodeDimension(node) / 3.0f * 3.0f;
			new_node->max.y = node->min.y + sNodeDimension(node) / 3.0f * 3.0f;
		}

		sSubdivideNode(new_node, min_node_dimension);
	}
}


/*-----------------------------

 NTerrainCreate()
-----------------------------*/
struct NTerrain* NTerrainCreate(const char* heightmap_filename, float elevation, float dimension,
                                float min_node_dimension, int pattern_subdivisions, struct Status* st)
{
	struct NTerrain* terrain = NULL;
	struct NTerrainNode* node = NULL;

	struct Image* heightmap = NULL;

	struct NTerrainState state = {0};
	struct NTerrainNode* last_parent = NULL;
	size_t parent_depth = 0;

	StatusSet(st, "NTerraiNCreate", STATUS_SUCCESS, NULL);

	if (heightmap_filename != NULL)
	{
		if ((heightmap = ImageLoad(heightmap_filename, st)) == NULL)
			goto return_failure;
	}

	if ((terrain = calloc(1, sizeof(struct NTerrain))) == NULL || (terrain->root = sNodeCreate(NULL)) == NULL)
	{
		StatusSet(st, "NTerraiNCreate", STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	terrain->dimension = dimension;
	terrain->elevation = elevation;

	// Mesures
	for (float i = dimension; i >= min_node_dimension; i /= 3.0f)
	{
		terrain->min_node_dimension = i;
		terrain->min_pattern_dimension = i;
		terrain->steps++;
	}

	for (int i = 0; i < pattern_subdivisions; i++)
		terrain->min_pattern_dimension /= 3.0f;

	// Node subdivision
	node = terrain->root;
	node->min = (struct Vector3){0.0, 0.0, 0.0};
	node->max = (struct Vector3){dimension, dimension, elevation};

	sSubdivideNode(terrain->root, min_node_dimension);

	// Pattern subdivision (the following big loop)
	// generating vertices and indexes
	struct Buffer aux_buffer = {0};
	state.start = terrain->root;

	while ((node = NTerrainIterate(&state, &aux_buffer, NULL)) != NULL)
	{
		terrain->nodes_no++;

		node->pattern_dimension = sNodeDimension(node);

		for (int i = 0; i < pattern_subdivisions; i++)
			node->pattern_dimension /= 3.0f;

		// Any parent has vertices to share?
		if (last_parent != NULL && state.depth > parent_depth)
		{
			node->vertices_type = INHERITED_FROM_PARENT;
			node->index = sGenerateIndex(&terrain->buffer, node, last_parent->min, last_parent->max,
			                             terrain->min_pattern_dimension);
		}
		else
		{
			// Can this node keep all childrens vertices? (because indices in OpenGL ES2)
			if (ceilf(powf(sNodeDimension(node) / terrain->min_pattern_dimension + 1.0f, 2.0)) < (float)UINT16_MAX)
			{
				last_parent = node;
				parent_depth = state.depth;
				terrain->vertices_buffers_no++;

				node->vertices_type = SHARED_WITH_CHILDRENS;
				node->vertices = sGenerateVertices(&terrain->buffer, node, terrain->min_pattern_dimension,
				                                   terrain->dimension, heightmap, elevation);
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
				                                   heightmap, elevation);
				node->index = sGenerateIndex(&terrain->buffer, node, node->min, node->max, node->pattern_dimension);
			}
		}
	}

	// Bye!
	BufferClean(&aux_buffer);
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
	struct NTerrainState state = {.start = terrain->root};
	struct NTerrainNode* node = NULL;

	if (terrain->root != NULL)
	{
		while ((node = NTerrainIterate(&state, &terrain->buffer, NULL)) != NULL)
		{
			IndexFree(&node->index);
			VerticesFree(&node->vertices);
			free(node);
		}
	}

	BufferClean(&terrain->buffer);
	free(terrain);
}


/*-----------------------------

 NTerrainIterate()
-----------------------------*/
static bool sCheckVisibility(const struct NTerrainNode* node, struct NTerrainView* view)
{
	// TODO: the other planes of the view pyramid, the back frustum
	// isn't really necessary, only four planes all call it a day

	// Front frustum (far = max_distance)
	if (sSphereBoxCollition(view->position, view->max_distance, node->min, node->max) == false)
		return false;

	// Back frustum (near = 0)
	struct Vector3 angle = {0};
	struct Vector3 cam_normal = {sinf(DegToRad(view->angle.z)) * sinf(DegToRad(view->angle.x)),
	                             cosf(DegToRad(view->angle.z)) * sinf(DegToRad(view->angle.x)),
	                             cosf(DegToRad(view->angle.x))};

	for (int i = 0; i < 8; i++)
	{
		switch (i)
		{
		case 0: angle = Vector3Subtract((struct Vector3){node->min.x, node->min.y, node->min.z}, view->position); break;
		case 1: angle = Vector3Subtract((struct Vector3){node->max.x, node->min.y, node->min.z}, view->position); break;
		case 2: angle = Vector3Subtract((struct Vector3){node->max.x, node->max.y, node->min.z}, view->position); break;
		case 3: angle = Vector3Subtract((struct Vector3){node->min.x, node->max.y, node->min.z}, view->position); break;

		case 4: angle = Vector3Subtract((struct Vector3){node->min.x, node->min.y, node->max.z}, view->position); break;
		case 5: angle = Vector3Subtract((struct Vector3){node->max.x, node->min.y, node->max.z}, view->position); break;
		case 6: angle = Vector3Subtract((struct Vector3){node->max.x, node->max.y, node->max.z}, view->position); break;
		case 7: angle = Vector3Subtract((struct Vector3){node->min.x, node->max.y, node->max.z}, view->position); break;
		}

		if (Vector3Dot(cam_normal, Vector3Normalize(angle)) <= 0.f)
			return true;
	}

	return false;
}

struct NTerrainNode* NTerrainIterate(struct NTerrainState* state, struct Buffer* buffer, struct NTerrainView* view)
{
	struct NTerrainNode** future_parent_heap = NULL;

	if (state->start != NULL)
	{
		state->future_depth = 1;
		state->future_return = state->start;
		state->actual = state->start;
		state->start = NULL;
	}

again:

	// Actual node
	if ((state->actual = state->future_return) == NULL)
		return NULL;

	state->depth = state->future_depth;
	state->future_return = NULL; // This one is resolved in next conditionals

	if (state->actual->vertices_type == SHARED_WITH_CHILDRENS || state->actual->vertices_type == OWN_VERTICES)
		state->last_with_vertices = state->actual;

	// Future values
	{
		// Go in? (childrens)
		float factor = sNodeDimension(state->actual);
		// factor = sNodeDimension(state->actual) * 2.0f; // Configurable quality

		if (state->actual->children != NULL &&
		    (view == NULL ||
		     sRectRectCollition((struct Vector2){view->position.x - factor / 2.0f, view->position.y - factor / 2.0f},
		                        (struct Vector2){view->position.x + factor / 2.0f, view->position.y + factor / 2.0f},
		                        (struct Vector2){state->actual->min.x, state->actual->min.y},
		                        (struct Vector2){state->actual->max.x, state->actual->max.y}) == true))
		{
			if (buffer->size < (state->depth + 1) * sizeof(void*))
				if (BufferResize(buffer, (state->depth + 1) * sizeof(void*)) == NULL) // FIXME, Bug in LibJapan, the +1
					return NULL;

			future_parent_heap = buffer->data;
			future_parent_heap[state->depth] = state->actual->next;
			state->future_return = state->actual->children;

			state->future_depth++;

			if (view != NULL)
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

	// The current node is apropiate for the specified view?
	if (view != NULL)
	{
		if (sCheckVisibility(state->actual, view) == false)
			goto again;

		// Determine if is at the border where LOD changes
		state->in_border = false;

		if (state->actual->parent != NULL) // The higher node didn't have a parent
		{
			state->in_border = true;

			float factor = sNodeDimension(state->actual->parent);

			struct Vector2 step = {view->position.x, view->position.y};
			step = Vector2Scale(step, 1.0f / sNodeDimension(state->actual->parent));
			step.x = roundf(step.x);
			step.y = roundf(step.y);
			step = Vector2Scale(step, sNodeDimension(state->actual->parent));

			if (sRectRectCollition((struct Vector2){step.x - factor / 2.0f, step.y - factor / 2.0f},
			                       (struct Vector2){step.x + factor / 2.0f, step.y + factor / 2.0f},
			                       (struct Vector2){state->actual->min.x, state->actual->min.y},
			                       (struct Vector2){state->actual->max.x, state->actual->max.y}) == true)
			{
				state->in_border = false;
			}
		}
	}

	return state->actual;
}


/*-----------------------------

 NTerrainDraw()
-----------------------------*/
int NTerrainDraw(struct Context* context, struct NTerrain* terrain, struct NTerrainView* view)
{
	struct NTerrainState state = {.start = terrain->root};

	struct NTerrainNode* node = NULL;
	struct NTerrainNode* temp = NULL;

	int dcalls = 0;

	while ((node = NTerrainIterate(&state, &terrain->buffer, view)) != NULL)
	{
		if (temp != state.last_with_vertices)
		{
			temp = state.last_with_vertices;

#ifndef TEST
			glBindBuffer(GL_ARRAY_BUFFER, state.last_with_vertices->vertices.glptr);
			glVertexAttribPointer(ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), NULL);
			glVertexAttribPointer(ATTRIBUTE_UV, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), (float*)NULL + 3);
			dcalls += 3;
#endif
		}

#ifndef TEST

		SetHighlight(context, (struct Vector3){0.0, 0.0, 0.0});
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, node->index.glptr);
		glDrawElements(GL_TRIANGLES, (GLsizei)node->index.length, GL_UNSIGNED_SHORT, NULL);

		if (state.in_border == true)
		{
			switch (state.depth)
			{
			case 0: SetHighlight(context, (struct Vector3){0.0, 0.0, 1.0}); break;
			case 1: SetHighlight(context, (struct Vector3){0.0, 1.0, 0.0}); break;
			case 2: SetHighlight(context, (struct Vector3){1.0, 1.0, 0.0}); break;
			case 3: SetHighlight(context, (struct Vector3){1.0, 0.25, 0.0}); break;
			default: SetHighlight(context, (struct Vector3){1.0, 0.0, 0.0});
			}

			glDrawElements(GL_LINES, (GLsizei)node->index.length, GL_UNSIGNED_SHORT, NULL);
		}

		dcalls += 2;
#endif
	}

	return dcalls;
}


/*-----------------------------

 NTerrainPrintInfo()
-----------------------------*/
void NTerrainPrintInfo(struct NTerrain* terrain)
{
	struct NTerrainState state = {.start = terrain->root};
	struct NTerrainNode* node = NULL;

	size_t prev_depth = 0;

	printf("Terrain 0x%p:\n", (void*)terrain);
	printf(" - Dimension: %f\n", terrain->dimension);
	printf(" - Minimum node dimension: %f\n", terrain->min_node_dimension);
	printf(" - Minimum pattern dimension: %f\n", terrain->min_pattern_dimension);
	printf(" - Steps: %lu\n", terrain->steps);
	printf(" - Nodes: %lu\n", terrain->nodes_no);
	printf(" - Vertices buffers: %lu\n", terrain->vertices_buffers_no);

	while ((node = NTerrainIterate(&state, &terrain->buffer, NULL)) != NULL)
	{
		if (prev_depth < state.depth || state.depth == 0)
		{
			printf(" # Step %lu:\n", state.depth);
			printf("    - Dimension: %f\n", (node->max.x - node->min.x));
			printf("    - Pattern dimension: %f\n", node->pattern_dimension);
			printf("    - Vertices type: %i\n", node->vertices_type);

			prev_depth = state.depth;
		}
	}
}
