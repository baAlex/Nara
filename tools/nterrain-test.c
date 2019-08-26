/*-----------------------------

 [nterrain-test.c]
 - Alexander Brandt 2019

 $ clang -I./source/lib-japan/include -I./source/engine/ ./source/lib-japan/source/buffer.c
./source/lib-japan/source/endianness.c ./source/lib-japan/source/status.c
./source/lib-japan/source/tree.c ./source/lib-japan/source/vector.c
./source/engine/nterrain.c ./tools/nterrain-test.c -lm -lcmocka -O0 -DTEST -g -o ./test

-----------------------------*/

#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <cmocka.h>

#ifndef TEST
#define TEST
#endif

#include "buffer.h"
#include "nterrain.h"

#define BORDER 10

enum CanvasInstructionType
{
	SET_OFFSET,
	SET_COLOR,
	DRAW_LINE
};

struct CanvasInstruction
{
	enum CanvasInstructionType type;

	union {
		struct Vector3 color;
		struct Vector2 offset;

		// DRAW_LINE
		struct
		{
			struct Vector2 a;
			struct Vector2 b;
		};
	};
};

struct Canvas
{
	size_t width;
	size_t height;

	struct Buffer instructions;
	size_t instructions_no;
};

static struct Canvas* sCanvasCreate(size_t width, size_t height)
{
	struct Canvas* canvas = NULL;

	if ((canvas = calloc(1, sizeof(struct Canvas))) != NULL)
	{
		canvas->width = width;
		canvas->height = height;
	}

	return canvas;
}

static void sCanvasDelete(struct Canvas* canvas)
{
	BufferClean(&canvas->instructions);
	free(canvas);
}

static void sCanvasSave(struct Canvas* canvas, const char* filename)
{
	// https://gitlab.gnome.org/GNOME/gimp/issues/2561

	FILE* fp = fopen(filename, "w");
	struct CanvasInstruction* instruction = canvas->instructions.data;

	struct Vector2 state_offset = {0};
	struct Vector3i state_color = {0};

	if (fp != NULL)
	{
		fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
		fprintf(fp, "<svg width=\"%zupx\" height=\"%zupx\" xmlns=\"http://www.w3.org/2000/svg\">\n", canvas->width,
		        canvas->height);

		fprintf(fp, "<rect x=\"-1\" y=\"-1\" width=\"%zu\" height=\"%zu\" fill=\"black\"/>\n", canvas->width + 2,
		        canvas->height + 2);

		for (size_t i = 0; i < canvas->instructions_no; i++)
		{
			switch (instruction->type)
			{
			case SET_OFFSET: state_offset = instruction->offset; break;

			case SET_COLOR:
				state_color.x = (int)((float)instruction->color.x * 255.0);
				state_color.y = (int)((float)instruction->color.y * 255.0);
				state_color.z = (int)((float)instruction->color.z * 255.0);
				break;

			case DRAW_LINE:
				fprintf(fp, "<line x1=\"%.02f\" y1=\"%.02f\" x2=\"%.02f\" y2=\"%.02f\" stroke=\"#%02X%02X%02X\"/>\n",
				        instruction->a.x + state_offset.x, instruction->a.y + state_offset.y,
				        instruction->b.x + state_offset.x, instruction->b.y + state_offset.y, state_color.x,
				        state_color.y, state_color.z);
				break;
			}

			instruction++;
		}

		fprintf(fp, "</svg>\n");
		fclose(fp);
	}
}

static void sCanvasSetOffset(struct Canvas* canvas, struct Vector2 offset)
{
	if (BufferResize(&canvas->instructions, (canvas->instructions_no + 1) * sizeof(struct CanvasInstruction)) != NULL)
	{
		canvas->instructions_no++;
		struct CanvasInstruction* instruction = canvas->instructions.data;
		instruction += (canvas->instructions_no - 1);

		instruction->type = SET_OFFSET;
		instruction->offset = offset;
	}
}

static void sCanvasSetColor(struct Canvas* canvas, struct Vector3 color)
{
	if (BufferResize(&canvas->instructions, (canvas->instructions_no + 1) * sizeof(struct CanvasInstruction)) != NULL)
	{
		canvas->instructions_no++;
		struct CanvasInstruction* instruction = canvas->instructions.data;
		instruction += (canvas->instructions_no - 1);

		instruction->type = SET_COLOR;
		instruction->color = color;
	}
}

static void sCanvasDrawLine(struct Canvas* canvas, struct Vector2 a, struct Vector2 b)
{
	if (BufferResize(&canvas->instructions, (canvas->instructions_no + 1) * sizeof(struct CanvasInstruction)) != NULL)
	{
		canvas->instructions_no++;
		struct CanvasInstruction* instruction = canvas->instructions.data;
		instruction += (canvas->instructions_no - 1);

		instruction->type = DRAW_LINE;
		instruction->a = a;
		instruction->b = b;
	}
}


/*-----------------------------

 sDrawTile()
-----------------------------*/
static void sDrawTile(struct Canvas* canvas, const struct NTerrainNode* node, const struct NTerrainNode* inherit_from)
{
	const struct Vertex* vertices;
	size_t index1 = 0;
	size_t index2 = 0;
	size_t index3 = 0;

	switch (node->vertices_type)
	{
	case OWN_VERTICES:
		sCanvasSetColor(canvas, (struct Vector3){0.5, 0.0, 0.0});
		vertices = node->vertices;
		break;
	case INHERITED_FROM_PARENT:
		sCanvasSetColor(canvas, (struct Vector3){0.5, 0.5, 0.5});
		vertices = (inherit_from != NULL) ? inherit_from->vertices : NULL;
		break;
	case SHARED_WITH_CHILDRENS:
		sCanvasSetColor(canvas, (struct Vector3){0.0, 0.5, 0.0});
		vertices = node->vertices;
		break;
	}

	// Inner triangles, omit if the density is lower that 10 'pixels'
	if (vertices != NULL && node->pattern_dimension > 10.0)
	{
		for (size_t i = 0; i < node->index_no; i += 3)
		{
			index1 = node->index[i];
			index2 = node->index[i + 1];
			index3 = node->index[i + 2];

			sCanvasDrawLine(canvas, (struct Vector2){vertices[index1].pos.x, vertices[index1].pos.y},
			                (struct Vector2){vertices[index2].pos.x, vertices[index2].pos.y});

			sCanvasDrawLine(canvas, (struct Vector2){vertices[index2].pos.x, vertices[index2].pos.y},
			                (struct Vector2){vertices[index3].pos.x, vertices[index3].pos.y});

			sCanvasDrawLine(canvas, (struct Vector2){vertices[index3].pos.x, vertices[index3].pos.y},
			                (struct Vector2){vertices[index1].pos.x, vertices[index1].pos.y});
		}
	}

	// Outside box
	// sCanvasSetColor(canvas, Vector3Scale(sCanvasGetColor(canvas), 2));
	sCanvasDrawLine(canvas, (struct Vector2){node->min.x, node->min.y}, (struct Vector2){node->min.x, node->max.y});
	sCanvasDrawLine(canvas, (struct Vector2){node->min.x, node->max.y}, (struct Vector2){node->max.x, node->max.y});
	sCanvasDrawLine(canvas, (struct Vector2){node->max.x, node->max.y}, (struct Vector2){node->max.x, node->min.y});
	sCanvasDrawLine(canvas, (struct Vector2){node->max.x, node->min.y}, (struct Vector2){node->min.x, node->min.y});
}


/*-----------------------------

 sDrawTerrainLayers()
-----------------------------*/
static void sDrawTerrainLayers(struct Canvas* canvas, struct NTerrain* terrain)
{
	struct TreeState s = {.start = terrain->root};
	struct Buffer buffer = {0};
	struct Tree* item = NULL;

	struct NTerrainNode* node = NULL;
	struct NTerrainNode* last_shared_node = NULL;

	while ((item = TreeIterate(&s, &buffer)) != NULL)
	{
		node = item->data;

		if (node->vertices_type == SHARED_WITH_CHILDRENS)
			last_shared_node = node;

		sCanvasSetOffset(canvas, (struct Vector2){BORDER, BORDER + (BORDER + terrain->dimension) * s.depth});
		sDrawTile(canvas, node, last_shared_node);
	}

	BufferClean(&buffer);
}


/*-----------------------------

 sDrawTerrainLod()
-----------------------------*/
static void sDrawTerrainLod(struct Canvas* canvas, struct NTerrain* terrain, struct Vector2 position)
{
	struct TreeState s = {.start = terrain->root};
	struct Buffer buffer = {0};

	struct NTerrainNode* node = NULL;
	struct NTerrainNode* last_shared_node = NULL;

	while ((node = NTerrainIterate(&s, &buffer, &last_shared_node, position)) != NULL)
	{
		sCanvasSetOffset(canvas, (struct Vector2){BORDER, BORDER});
		sDrawTile(canvas, node, last_shared_node);
	}

	BufferClean(&buffer);
}


/*-----------------------------

 sPrintInfo()
-----------------------------*/
static void sPrintInfo(struct NTerrain* terrain)
{
	struct TreeState s = {.start = terrain->root};
	struct Buffer buffer = {0};
	struct Tree* item = NULL;
	struct NTerrainNode* node = NULL;
	size_t prev_depth = 0;

	printf("Terrain 0x%p:\n", (void*)terrain);
	printf(" - Dimension: %0.4f\n", terrain->dimension);
	printf(" - Minimum tile dimension: %0.4f\n", terrain->min_tile_dimension);
	printf(" - Minimum pattern dimension: %0.4f\n", terrain->min_pattern_dimension);
	printf(" - Steps: %lu\n", terrain->steps);
	printf(" - Tiles: %lu\n", terrain->tiles_no);
	printf(" - Vertices buffers: %lu\n", terrain->vertices_buffers_no);

	while ((item = TreeIterate(&s, &buffer)) != NULL)
	{
		node = item->data;

		if (prev_depth < s.depth || s.depth == 0)
		{
			printf(" # Step %lu:\n", s.depth);
			printf("    - Dimension: %0.4f\n", (node->max.x - node->min.x));
			printf("    - Pattern dimension: %0.4f\n", node->pattern_dimension);
			printf("    - Index length: %lu\n", node->index_no);
			printf("    - Vertices type: %i\n", node->vertices_type);

			prev_depth = s.depth;
		}
	}
}


/*-----------------------------

 TestNormalTerrain()
-----------------------------*/
void TestNormalTerrain(void** cmocka_state)
{
// Arbitrary values that did not goes well with subdivisions of three
// that the terrain code do.
#define TERRAIN_DIMENSION 1024.0
#define TILE_DIMENSION 64.0
#define PATTERN_SUBDIVISION 2

	struct NTerrain* terrain = NULL;
	struct Canvas* c = NULL;
	struct Status st = {0};

	struct timespec start_time = {0};
	struct timespec end_time = {0};
	double ms = 0.0;

	timespec_get(&start_time, TIME_UTC);

	if ((terrain = NTerrainCreate(TERRAIN_DIMENSION, TILE_DIMENSION, PATTERN_SUBDIVISION, &st)) == NULL)
	{
		StatusPrint("TestNormalTerrain", st);
		assert_int_equal(st.code, STATUS_SUCCESS);
	}

	// On early writtings, all the recursive operations took like 100 ms. I can assume
	// that not only was fault of a large push on the stack, but the fact that the code
	// did not fit on the Cpu cache. After separating the tile subdivision step from the
	// pattern subdivision step on the NTerrainCreate() function, the numbers turned
	// pretty again. Of course I am not really sure, but is a good idea to control the time.
	timespec_get(&end_time, TIME_UTC);
	ms = end_time.tv_nsec / 1000000.0 + end_time.tv_sec * 1000.0;
	ms -= start_time.tv_nsec / 1000000.0 + start_time.tv_sec * 1000.0;

	printf("Done in %0.4f ms\n", ms);

	sPrintInfo(terrain);

	if ((c = sCanvasCreate(terrain->dimension + BORDER * 2, BORDER + (terrain->dimension + BORDER) * terrain->steps)) !=
	    NULL)
	{
		sDrawTerrainLayers(c, terrain);
		sCanvasSave(c, "./TestNormalTerrain.svg");
		sCanvasDelete(c);
	}

	NTerrainDelete(terrain);

#undef TERRAIN_DIMENSION
#undef TILE_DIMENSION
#undef PATTERN_SUBDIVISION
}


/*-----------------------------

 TestPreciseTerrain()
-----------------------------*/
void TestPreciseTerrain(void** cmocka_state)
{
	// The following values where an hide trouble for sSubdivideTile(). The less
	// or equal comparison: `if ((node_dimension / 3.0) <= min_tile_dimension)`
	// fixed them. But I'm still worried about the loss of precision.

	// EDIT: With vertices, buffers plus the tree node, the mallocs skyrocketed.
	// Compare the numbers against sPrintInfo().

	// [OLD] Put an eye on the mallocs operations number that Valgrind reports. From
	// an normal of ~700 it jumps to ~7000. In other cases it simply stall the heap.

#define TERRAIN_DIMENSION 972.0
#define TILE_DIMENSION 12.0
#define PATTERN_SUBDIVISION 3

	struct NTerrain* terrain = NULL;
	struct Canvas* c = NULL;
	struct Status st = {0};

	struct timespec start_time = {0};
	struct timespec end_time = {0};
	double ms = 0.0;

	timespec_get(&start_time, TIME_UTC);

	if ((terrain = NTerrainCreate(TERRAIN_DIMENSION, TILE_DIMENSION, PATTERN_SUBDIVISION, &st)) == NULL)
	{
		StatusPrint("TestPreciseTerrain", st);
		assert_int_equal(st.code, STATUS_SUCCESS);
	}

	timespec_get(&end_time, TIME_UTC);
	ms = end_time.tv_nsec / 1000000.0 + end_time.tv_sec * 1000.0;
	ms -= start_time.tv_nsec / 1000000.0 + start_time.tv_sec * 1000.0;

	printf("Done in %0.4f ms\n", ms);

	sPrintInfo(terrain);

	if ((c = sCanvasCreate(terrain->dimension + BORDER * 2, BORDER + (terrain->dimension + BORDER) * terrain->steps)) !=
	    NULL)
	{
		sDrawTerrainLayers(c, terrain);
		sCanvasSave(c, "./TestPreciseTerrain.svg");
		sCanvasDelete(c);
	}

	NTerrainDelete(terrain);

#undef TERRAIN_DIMENSION
#undef TILE_DIMENSION
#undef PATTERN_SUBDIVISION
}


/*-----------------------------

 TestIteration1()
-----------------------------*/
void TestIteration1(void** cmocka_state)
{
#define TERRAIN_DIMENSION 972.0
#define TILE_DIMENSION 6.0
#define PATTERN_SUBDIVISION 2

	struct NTerrain* terrain = NULL;
	struct Canvas* c = NULL;
	struct Status st = {0};

	if ((terrain = NTerrainCreate(TERRAIN_DIMENSION, TILE_DIMENSION, PATTERN_SUBDIVISION, &st)) == NULL)
	{
		StatusPrint("TestIteration", st);
		assert_int_equal(st.code, STATUS_SUCCESS);
	}

	sPrintInfo(terrain);

	if ((c = sCanvasCreate(terrain->dimension + BORDER * 2, terrain->dimension + BORDER * 2)) != NULL)
	{
		sDrawTerrainLod(c, terrain, (struct Vector2){terrain->dimension / 3, terrain->dimension / 2});
		sCanvasSave(c, "./TestIteration1.svg");
		sCanvasDelete(c);
	}

	NTerrainDelete(terrain);

#undef TERRAIN_DIMENSION
#undef TILE_DIMENSION
#undef PATTERN_SUBDIVISION
}


/*-----------------------------

 TestIteration2()
-----------------------------*/
void TestIteration2(void** cmocka_state)
{
#define TERRAIN_DIMENSION 972.0
#define TILE_DIMENSION 6.0
#define PATTERN_SUBDIVISION 2

	struct NTerrain* terrain = NULL;
	struct Canvas* c = NULL;
	struct Status st = {0};

	if ((terrain = NTerrainCreate(TERRAIN_DIMENSION, TILE_DIMENSION, PATTERN_SUBDIVISION, &st)) == NULL)
	{
		StatusPrint("TestIteration", st);
		assert_int_equal(st.code, STATUS_SUCCESS);
	}

	if ((c = sCanvasCreate(terrain->dimension + BORDER * 2, terrain->dimension + BORDER * 2)) != NULL)
	{
		sDrawTerrainLod(c, terrain, (struct Vector2){terrain->dimension, terrain->dimension});
		sCanvasSave(c, "./TestIteration2.svg");
		sCanvasDelete(c);
	}

	NTerrainDelete(terrain);

#undef TERRAIN_DIMENSION
#undef TILE_DIMENSION
#undef PATTERN_SUBDIVISION
}


/*-----------------------------

 main()
-----------------------------*/
int main()
{
	const struct CMUnitTest tests[] = {cmocka_unit_test(TestNormalTerrain), cmocka_unit_test(TestPreciseTerrain),
	                                   cmocka_unit_test(TestIteration1), cmocka_unit_test(TestIteration2)};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
