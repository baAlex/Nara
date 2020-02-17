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
 - Alexander Brandt 2019-2020
-----------------------------*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "japan-utilities.h"

#include "misc.h"
#include "nterrain.h"


static inline float sNodeDimension(const struct NTerrainNode* node)
{
	return (node->bbox.max.x - node->bbox.min.x);
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
static float sHeightmapElevation(const struct jaImage* hmap, struct jaVector2 cords)
{
	// https://en.wikipedia.org/wiki/Bilinear_filtering
	union {
		uint8_t* u8;
		uint16_t* u16;
		void* raw;

	} data = {.raw = hmap->data};

	cords.x = jaClamp(cords.x, -1.0f, 1.0f);
	cords.y = jaClamp(cords.y, -1.0f, 1.0f);

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
	case IMAGE_U8:
		result = (data.u8[x + hmap->width * y] * u_opposite + data.u8[x + 1 + hmap->width * y] * u_ratio) * v_opposite +
		         (data.u8[x + hmap->width * (y + 1)] * u_opposite + data.u8[x + 1 + hmap->width * (y + 1)] * u_ratio) *
		             v_ratio;
		result /= 255.0f;
		break;

	case IMAGE_U16:
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

float NTerrainElevation(const struct NTerrain* terrain, struct jaVector2 cords)
{
	return sHeightmapElevation(terrain->heightmap, jaVector2Scale(cords, 1.0f / terrain->dimension)) *
	       terrain->elevation;
}


/*-----------------------------

 sGenerateIndex()
-----------------------------*/
static struct Index sGenerateIndex(struct jaBuffer* buffer, const struct NTerrainNode* node, struct jaAABBox org,
                                   float org_pattern_dimension)
{
	// Generation on a temporary buffer
	size_t const patterns_no = sPatternsInARow(sNodeDimension(node), node->pattern_dimension);
	size_t const length = patterns_no * patterns_no * 6; // 6 = Two triangles vertices, as OpenGL ES2 did not have quads
	size_t col = 0;

	if (buffer->size < sizeof(uint16_t) * length)
		jaBufferResize(buffer, sizeof(uint16_t) * length); // TODO, check failure

	uint16_t* temp_data = buffer->data;

	{
		size_t const org_patterns_no = sPatternsInARow(org.max.x - org.min.x, org_pattern_dimension);
		size_t const org_step =
		    (org_patterns_no / patterns_no) / (size_t)lroundf((org.max.x - org.min.x) / sNodeDimension(node));

		size_t const org_col_start = sPatternsInARow(node->bbox.min.x - org.min.x, org_pattern_dimension);
		size_t org_row = sPatternsInARow(node->bbox.min.y - org.min.y, org_pattern_dimension);
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
	IndexInit(temp_data, length, &index, NULL); // TODO: Check error

	return index;
}


/*-----------------------------

 sGenerateVertices()
-----------------------------*/
static struct Vertices sGenerateVertices(struct jaBuffer* buffer, const struct NTerrainNode* node,
                                         float pattern_dimension, float terrain_dimension, const struct jaImage* hmap,
                                         float elevation)
{
	// Generation on a temporary buffer
	size_t const patterns_no = sPatternsInARow(sNodeDimension(node), pattern_dimension);

	if (buffer->size < sizeof(struct Vertex) * (patterns_no + 1) * (patterns_no + 1))
		jaBufferResize(buffer, sizeof(struct Vertex) * (patterns_no + 1) * (patterns_no + 1));

	struct Vertex* temp_data = buffer->data;

	{
		struct jaVector2 texture_step = {node->bbox.min.x / terrain_dimension, node->bbox.min.y / terrain_dimension};

		for (size_t row = 0; row < (patterns_no + 1); row++)
		{
			for (size_t col = 0; col < (patterns_no + 1); col++)
			{
				temp_data[col + (patterns_no + 1) * row].uv.x = texture_step.x;
				temp_data[col + (patterns_no + 1) * row].uv.y = texture_step.y;

				temp_data[col + (patterns_no + 1) * row].pos.x = node->bbox.min.x + (float)col * pattern_dimension;
				temp_data[col + (patterns_no + 1) * row].pos.y = node->bbox.min.y + (float)row * pattern_dimension;

				if (hmap != NULL)
					temp_data[col + (patterns_no + 1) * row].pos.z =
					    sHeightmapElevation(hmap, texture_step) * elevation;
				else
					temp_data[col + (patterns_no + 1) * row].pos.z = 0.0;

				// Next step
				texture_step.x += pattern_dimension / terrain_dimension;
			}

			texture_step.y += pattern_dimension / terrain_dimension;
			texture_step.x = node->bbox.min.x / terrain_dimension;
		}
	}

	// Copy to the final index
	struct Vertices vertices = {0};
	VerticesInit(temp_data, (uint16_t)((patterns_no + 1) * (patterns_no + 1)), &vertices, NULL); // TODO: Check error

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

		new_node->bbox.min.z = node->bbox.min.z;
		new_node->bbox.max.z = node->bbox.max.z;

		switch (i)
		{
		case 0: // North West
			new_node->bbox.min.x = node->bbox.min.x;
			new_node->bbox.min.y = node->bbox.min.y;
			new_node->bbox.max.x = node->bbox.min.x + sNodeDimension(node) / 3.0f;
			new_node->bbox.max.y = node->bbox.min.y + sNodeDimension(node) / 3.0f;
			break;
		case 1: // North
			new_node->bbox.min.x = node->bbox.min.x + sNodeDimension(node) / 3.0f;
			new_node->bbox.min.y = node->bbox.min.y;
			new_node->bbox.max.x = node->bbox.min.x + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->bbox.max.y = node->bbox.min.y + sNodeDimension(node) / 3.0f;
			break;
		case 2: // North East
			new_node->bbox.min.x = node->bbox.min.x + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->bbox.min.y = node->bbox.min.y;
			new_node->bbox.max.x = node->bbox.min.x + sNodeDimension(node) / 3.0f * 3.0f;
			new_node->bbox.max.y = node->bbox.min.y + sNodeDimension(node) / 3.0f;
			break;
		case 3: // West
			new_node->bbox.min.x = node->bbox.min.x;
			new_node->bbox.min.y = node->bbox.min.y + sNodeDimension(node) / 3.0f;
			new_node->bbox.max.x = node->bbox.min.x + sNodeDimension(node) / 3.0f;
			new_node->bbox.max.y = node->bbox.min.y + sNodeDimension(node) / 3.0f * 2.0f;
			break;
		case 4: // Center
			new_node->bbox.min.x = node->bbox.min.x + sNodeDimension(node) / 3.0f;
			new_node->bbox.min.y = node->bbox.min.y + sNodeDimension(node) / 3.0f;
			new_node->bbox.max.x = node->bbox.min.x + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->bbox.max.y = node->bbox.min.y + sNodeDimension(node) / 3.0f * 2.0f;
			break;
		case 5: // East
			new_node->bbox.min.x = node->bbox.min.x + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->bbox.min.y = node->bbox.min.y + sNodeDimension(node) / 3.0f;
			new_node->bbox.max.x = node->bbox.min.x + sNodeDimension(node) / 3.0f * 3.0f;
			new_node->bbox.max.y = node->bbox.min.y + sNodeDimension(node) / 3.0f * 2.0f;
			break;
		case 6: // South West
			new_node->bbox.min.x = node->bbox.min.x;
			new_node->bbox.min.y = node->bbox.min.y + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->bbox.max.x = node->bbox.min.x + sNodeDimension(node) / 3.0f;
			new_node->bbox.max.y = node->bbox.min.y + sNodeDimension(node) / 3.0f * 3.0f;
			break;
		case 7: // South
			new_node->bbox.min.x = node->bbox.min.x + sNodeDimension(node) / 3.0f;
			new_node->bbox.min.y = node->bbox.min.y + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->bbox.max.x = node->bbox.min.x + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->bbox.max.y = node->bbox.min.y + sNodeDimension(node) / 3.0f * 3.0f;
			break;
		case 8: // South East
			new_node->bbox.min.x = node->bbox.min.x + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->bbox.min.y = node->bbox.min.y + sNodeDimension(node) / 3.0f * 2.0f;
			new_node->bbox.max.x = node->bbox.min.x + sNodeDimension(node) / 3.0f * 3.0f;
			new_node->bbox.max.y = node->bbox.min.y + sNodeDimension(node) / 3.0f * 3.0f;
		}

		sSubdivideNode(new_node, min_node_dimension);
	}
}


/*-----------------------------

 NTerrainCreate()
-----------------------------*/
struct NTerrain* NTerrainCreate(const char* heightmap_filename, float elevation, float dimension,
                                float min_node_dimension, int pattern_subdivisions, struct jaStatus* st)
{
	struct NTerrain* terrain = NULL;
	struct NTerrainNode* node = NULL;

	struct NTerrainState state = {0};
	struct NTerrainNode* last_parent = NULL;
	size_t parent_depth = 0;

	jaStatusSet(st, "NTerraiNCreate", STATUS_SUCCESS, NULL);

	if ((terrain = calloc(1, sizeof(struct NTerrain))) == NULL || (terrain->root = sNodeCreate(NULL)) == NULL)
	{
		jaStatusSet(st, "NTerraiNCreate", STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	terrain->dimension = dimension;
	terrain->elevation = elevation;

	if ((terrain->heightmap = jaImageLoad(heightmap_filename, st)) == NULL)
		goto return_failure;

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
	node->bbox.min = (struct jaVector3){0.0f, 0.0f, 0.0f};
	node->bbox.max = (struct jaVector3){dimension, dimension, elevation};

	sSubdivideNode(terrain->root, min_node_dimension);

	// Pattern subdivision (the following big loop)
	// generating vertices and indexes
	struct jaBuffer aux_buffer = {0};
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
			node->index = sGenerateIndex(&terrain->buffer, node, last_parent->bbox, terrain->min_pattern_dimension);
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
				                                   terrain->dimension, terrain->heightmap, elevation);
				node->index = sGenerateIndex(&terrain->buffer, node, node->bbox, terrain->min_pattern_dimension);
			}

			// Nope, only his own vertices
			else
			{
				last_parent = NULL;
				parent_depth = 0;
				terrain->vertices_buffers_no++;

				node->vertices_type = OWN_VERTICES;
				node->vertices = sGenerateVertices(&terrain->buffer, node, node->pattern_dimension, terrain->dimension,
				                                   terrain->heightmap, elevation);
				node->index = sGenerateIndex(&terrain->buffer, node, node->bbox, node->pattern_dimension);
			}
		}
	}

	// Bye!
	jaBufferClean(&aux_buffer);
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
	if (terrain == NULL)
		return;

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

	jaBufferClean(&terrain->buffer);
	free(terrain);
}


/*-----------------------------

 NTerrainIterate()
-----------------------------*/
static inline bool sCheckPlane(const struct jaAABBox bbox, struct jaVector3 pos, struct jaVector3 nor)
{
	struct jaVector3 angle = {0};

	for (int i = 0; i < 8; i++)
	{
		switch (i)
		{
		case 0: angle = jaVector3Subtract((struct jaVector3){bbox.min.x, bbox.min.y, bbox.min.z}, pos); break;
		case 1: angle = jaVector3Subtract((struct jaVector3){bbox.max.x, bbox.min.y, bbox.min.z}, pos); break;
		case 2: angle = jaVector3Subtract((struct jaVector3){bbox.max.x, bbox.max.y, bbox.min.z}, pos); break;
		case 3: angle = jaVector3Subtract((struct jaVector3){bbox.min.x, bbox.max.y, bbox.min.z}, pos); break;

		case 4: angle = jaVector3Subtract((struct jaVector3){bbox.min.x, bbox.min.y, bbox.max.z}, pos); break;
		case 5: angle = jaVector3Subtract((struct jaVector3){bbox.max.x, bbox.min.y, bbox.max.z}, pos); break;
		case 6: angle = jaVector3Subtract((struct jaVector3){bbox.max.x, bbox.max.y, bbox.max.z}, pos); break;
		case 7: angle = jaVector3Subtract((struct jaVector3){bbox.min.x, bbox.max.y, bbox.max.z}, pos); break;
		}

		if (jaVector3Dot(nor, jaVector3Normalize(angle)) >= 0.f)
			return true;
	}

	return false;
}

static void sBuildFrustumPlanes(struct NTerrainState* state, const struct NTerrainView* view)
{
	// http://www.lighthouse3d.com/tutorials/view-frustum-culling/geometric-approach-extracting-the-planes/

	// TODO: beyond 90º the fov stops working... definitively PI is the problem

	// TODO: Dear future Alex, please study about the cross operation and why
	// the right frustum plane require an inverted up vector. :)

	struct jaVector3 forward;
	struct jaVector3 left;
	struct jaVector3 up;

	VectorAxes(view->angle, &forward, &left, &up);

	// Front frustum
	state->frustum_position[0] = jaVector3Add(view->position, jaVector3Scale(forward, view->max_distance));
	state->frustum_normal[0] = jaVector3Invert(forward);

	// Left frustum
	state->frustum_position[2] = jaVector3Add(view->position, jaVector3Scale(forward, 1.0f));
	state->frustum_position[2] = jaVector3Subtract(
	    state->frustum_position[2], jaVector3Scale(left, (jaRadToDeg(view->fov) / 90.0f) * view->aspect));

	state->frustum_normal[2] = jaVector3Subtract(state->frustum_position[2], view->position);
	state->frustum_normal[2] = jaVector3Cross(jaVector3Normalize(state->frustum_normal[2]), up);

	// Right frustum
	state->frustum_position[1] = jaVector3Add(view->position, jaVector3Scale(forward, 1.0f));
	state->frustum_position[1] =
	    jaVector3Add(state->frustum_position[1], jaVector3Scale(left, (jaRadToDeg(view->fov) / 90.0f) * view->aspect));

	state->frustum_normal[1] = jaVector3Subtract(state->frustum_position[1], view->position);
	state->frustum_normal[1] = jaVector3Cross(jaVector3Normalize(state->frustum_normal[1]), jaVector3Invert(up));

	// Top frustum
	state->frustum_position[3] = jaVector3Add(view->position, jaVector3Scale(forward, 1.0f));
	state->frustum_position[3] =
	    jaVector3Add(state->frustum_position[3], jaVector3Scale(up, jaRadToDeg(view->fov) / 90.0f));

	state->frustum_normal[3] = jaVector3Subtract(state->frustum_position[3], view->position);
	state->frustum_normal[3] = jaVector3Cross(jaVector3Normalize(state->frustum_normal[3]), left);

	// Bottom frustum
	state->frustum_position[4] = jaVector3Add(view->position, jaVector3Scale(forward, 1.0f));
	state->frustum_position[4] =
	    jaVector3Subtract(state->frustum_position[4], jaVector3Scale(up, jaRadToDeg(view->fov) / 90.0f));

	state->frustum_normal[4] = jaVector3Subtract(state->frustum_position[4], view->position);
	state->frustum_normal[4] = jaVector3Cross(jaVector3Normalize(state->frustum_normal[4]), jaVector3Invert(left));
}

struct NTerrainNode* NTerrainIterate(struct NTerrainState* state, struct jaBuffer* buffer, struct NTerrainView* view)
{
	struct NTerrainNode** future_parent_heap = NULL;

	if (state->start != NULL)
	{
		state->future_depth = 1;
		state->future_return = state->start;
		state->actual = state->start;
		state->start = NULL;

		if (view != NULL)
			sBuildFrustumPlanes(state, view);
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
		bool view_condition = false;

		if (view != NULL)
		{
			const float factor = sNodeDimension(state->actual) * (float)view->lod_factor;

			view_condition = jaAABCollisionBoxBox(
			    ((struct jaAABBox){
			        jaVector3Subtract(view->position, (struct jaVector3){factor / 2.0f, factor / 2.0f, factor / 2.0f}),
			        jaVector3Add(view->position, (struct jaVector3){factor / 2.0f, factor / 2.0f, factor / 2.0f})}),
			    state->actual->bbox);
		}
		else
			view_condition = true; // The user didn't provided a view, so we iterate every children

		if (state->actual->children != NULL && view_condition == true)
		{
			if (buffer->size < (state->depth + 1) * sizeof(void*))
			{
				if (jaBufferResize(buffer, (state->depth + 1) * sizeof(void*)) == NULL)
					return NULL;
			}

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

	// View based checks
	if (view != NULL)
	{
		// Check visibility against frustum planes
		for (int f = 0; f < 5; f++)
		{
			if (sCheckPlane(state->actual->bbox, state->frustum_position[f], state->frustum_normal[f]) == false)
				goto again;
		}

		// Determine if is at the border where LOD changes
		state->in_border = false;

		if (state->actual->parent != NULL) // The higher node didn't have a parent
		{
			state->in_border = true;

			// Lets create a 'view_rectangle', similar to the box that determines
			// when a node should be subdivided (the 'Go in?' step). Also, note
			// that 'factor' is the same, the difference is that this rectangle has
			// his coordinates aligned.

			float factor = sNodeDimension(state->actual->parent) * (float)view->lod_factor;

			struct jaVector2 align = {view->position.x, view->position.y};
			align = jaVector2Scale(align, 1.0f / (sNodeDimension(state->actual->parent)));
			align.x = roundf(align.x);
			align.y = roundf(align.y);
			align = jaVector2Scale(align, (sNodeDimension(state->actual->parent)));

			state->view_rectangle = (struct jaAABRectangle){{align.x - factor / 2.0f, align.y - factor / 2.0f},
			                                                {align.x + factor / 2.0f, align.y + factor / 2.0f}};

			if (jaAABCollisionRectRect(state->view_rectangle, jaAABToRectangle(state->actual->bbox)) == true)
				state->in_border = false;
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

#ifndef TEST
	SetHighlight(context, (struct jaVector3){0.0f, 0.0f, 0.0f});

	while ((node = NTerrainIterate(&state, &terrain->buffer, view)) != NULL)
	{
		if (temp != state.last_with_vertices)
		{
			temp = state.last_with_vertices;

			SetVertices(context, &state.last_with_vertices->vertices);
			dcalls += 1;
		}

		if (state.in_border == false)
		{
			Draw(context, &node->index);
			dcalls += 1;
		}
		else
		{
			/*switch (state.depth)
			{
			case 0: SetHighlight(context, (struct jaVector3){0.0, 0.0, 1.0}); break;
			case 1: SetHighlight(context, (struct jaVector3){0.0, 1.0, 0.0}); break;
			case 2: SetHighlight(context, (struct jaVector3){1.0, 1.0, 0.0}); break;
			case 3: SetHighlight(context, (struct jaVector3){1.0, 0.25, 0.0}); break;
			default: SetHighlight(context, (struct jaVector3){1.0, 0.0, 0.0});
			}*/

			Draw(context, &node->index);
			dcalls += 1;

			// SetHighlight(context, (struct jaVector3){0.0f, 0.0f, 0.0f});
		}
	}
#endif

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
			printf("    - Dimension: %f\n", (node->bbox.max.x - node->bbox.min.x));
			printf("    - Pattern dimension: %f\n", node->pattern_dimension);
			printf("    - Vertices type: %i\n", node->vertices_type);

			prev_depth = state.depth;
		}
	}

	printf("\n");
}
