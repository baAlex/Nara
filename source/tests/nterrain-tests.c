/*-----------------------------

 [nterrain-tests.c]
 - Alexander Brandt 2019
-----------------------------*/

#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "canvas.h"
#include <cmocka.h>

#include "../engine/nterrain.h"
#include "../engine/tiny-gl.h" // Mocked


int VerticesInit(struct Vertices* out, const struct Vertex* data, size_t length, struct Status* st)
{
	(void)data;
	out->glptr = 0;
	out->length = length;
	StatusSet(st, "VerticesInit", STATUS_SUCCESS, NULL);
	return 0;
}

void VerticesFree(struct Vertices* vertices)
{
	(void)vertices;
}

int IndexInit(struct Index* out, const uint16_t* data, size_t length, struct Status* st)
{
	(void)data;
	out->glptr = 0;
	out->length = length;
	StatusSet(st, "VerticesInit", STATUS_SUCCESS, NULL);
	return 0;
}

void IndexFree(struct Index* index)
{
	(void)index;
}

static inline bool sFloatEquals(float a, float b)
{
	// https://en.wikipedia.org/wiki/Machine_epsilon
	return fabs(a - b) < 1.19e-07f;
}

static inline bool sFloatRoughtEquals(float a, float b)
{
	return fabs(a - b) < 0.1f;
}


/*-----------------------------

 sDrawTile()
-----------------------------*/
static void sDrawTile(struct Canvas* canvas, const struct NTerrainNode* node, const struct NTerrainNode* vertices_from)
{
	(void)vertices_from;

	switch (node->vertices_type)
	{
	case OWN_VERTICES: CanvasSetColor(canvas, (struct Vector3){0.5, 0.0, 0.0}); break;
	case INHERITED_FROM_PARENT: CanvasSetColor(canvas, (struct Vector3){0.5, 0.5, 0.5}); break;
	case SHARED_WITH_CHILDRENS: CanvasSetColor(canvas, (struct Vector3){0.0, 0.5, 0.0}); break;
	}

	// Inner triangles
	{
		// TODO, currently the engine did not keep the triangles
	}

	// Outside box
	CanvasSetColor(canvas, Vector3Scale(CanvasGetColor(canvas), 2));
	CanvasDrawLine(canvas, (struct Vector2){node->min.x, node->min.y}, (struct Vector2){node->min.x, node->max.y});
	CanvasDrawLine(canvas, (struct Vector2){node->min.x, node->max.y}, (struct Vector2){node->max.x, node->max.y});
	CanvasDrawLine(canvas, (struct Vector2){node->max.x, node->max.y}, (struct Vector2){node->max.x, node->min.y});
	CanvasDrawLine(canvas, (struct Vector2){node->max.x, node->min.y}, (struct Vector2){node->min.x, node->min.y});
}


/*-----------------------------

 sDrawTerrainLayers()
-----------------------------*/
#define BORDER 10

void sDrawTerrainLayers(struct NTerrain* terrain, const char* filename)
{
	struct TreeState s = {.start = terrain->root};
	struct Tree* item = NULL;

	struct NTerrainNode* node = NULL;
	struct NTerrainNode* last_with_vertices = NULL;

	struct Canvas* canvas = NULL;
	size_t const canvas_size_x = (size_t)terrain->dimension + BORDER * 2;
	size_t const canvas_size_y = ((size_t)terrain->dimension + BORDER) * terrain->steps + BORDER;

	if ((canvas = CanvasCreate(canvas_size_x, canvas_size_y)) != NULL)
	{
		while ((item = TreeIterate(&s, &terrain->buffer)) != NULL)
		{
			node = item->data;

			if (node->vertices_type == SHARED_WITH_CHILDRENS)
				last_with_vertices = node;

			CanvasSetOffset(canvas, (struct Vector2){BORDER, BORDER + (BORDER + terrain->dimension) * (float)s.depth});
			sDrawTile(canvas, node, last_with_vertices);
		}

		CanvasSave(canvas, filename);
		CanvasDelete(canvas);
	}
}


/*-----------------------------

 sPrintInfo()
-----------------------------*/
static void sPrintInfo(const struct NTerrain* terrain)
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


/*-----------------------------

 TestMesures1()
-----------------------------*/
void TestMesures1(void** cmocka_state)
{
	(void)cmocka_state;
	struct NTerrain* terrain = NULL;
	struct Status st = {0};

	float const ELEVATION = 100.0f;
	float const DIMENSION = 1024.0f;
	float const MIN_TILE_DIMENSION = 64.0f;
	int const PATTERN_SUBDIVISIONS = 0;

	terrain = NTerrainCreate(NULL, ELEVATION, DIMENSION, MIN_TILE_DIMENSION, PATTERN_SUBDIVISIONS, &st);

	if (terrain == NULL)
	{
		StatusPrint("TestMesures1", st);
		assert_int_equal(st.code, STATUS_SUCCESS);
	}

	sPrintInfo(terrain);

	// Take a calculator
	{
		assert_true((terrain->min_tile_dimension > MIN_TILE_DIMENSION) ||
		            sFloatEquals(terrain->min_tile_dimension, MIN_TILE_DIMENSION));

		// Value before (or equal) 64, dividing 1024 by three
		assert_true(sFloatEquals(terrain->min_tile_dimension, 113.77778f));

		// Steps at dividing by three: 1024, 341, 113
		assert_true(terrain->steps == 3);

		// Previous subdivisions in 2d
		// (1024/1024)^2 + (1024/341)^2 + (1024/113)^2
		assert_true(terrain->tiles_no == 91);

		struct TreeState s = {.start = terrain->root};
		struct Tree* item = NULL;
		struct NTerrainNode* node = NULL;

		while ((item = TreeIterate(&s, &terrain->buffer)) != NULL)
		{
			node = item->data;

			// If tiles are squares, note that I'm using sFloatRoughtEquals()
			if (sFloatRoughtEquals((node->max.x - node->min.x), (node->max.y - node->min.y)) == false)
				printf("%f != %f\n", (node->max.x - node->min.x), (node->max.y - node->min.y));

			assert_true(sFloatRoughtEquals((node->max.x - node->min.x), (node->max.y - node->min.y)));

			// If dimensions follows the terrain subdivision
			assert_true(sFloatRoughtEquals((node->max.x - node->min.x), 1024.0f) ||
			            sFloatRoughtEquals((node->max.x - node->min.x), 341.33334f) ||
			            sFloatRoughtEquals((node->max.x - node->min.x), 113.77778f));
		}
	}

	if (getenv("I_WANT_A_SOUVENIR") != NULL)
		sDrawTerrainLayers(terrain, "TestMesures1.svg");

	NTerrainDelete(terrain);
}


/*-----------------------------

 TestMesures2()
-----------------------------*/
void TestMesures2(void** cmocka_state)
{
	(void)cmocka_state;
	struct NTerrain* terrain = NULL;
	struct Status st = {0};

	float const ELEVATION = 100.0f;
	float const DIMENSION = 972.0f;
	float const MIN_TILE_DIMENSION = 12.0f;
	int const PATTERN_SUBDIVISIONS = 0;

	terrain = NTerrainCreate(NULL, ELEVATION, DIMENSION, MIN_TILE_DIMENSION, PATTERN_SUBDIVISIONS, &st);

	if (terrain == NULL)
	{
		StatusPrint("TestMesures2", st);
		assert_int_equal(st.code, STATUS_SUCCESS);
	}

	sPrintInfo(terrain);

	// Take a calculator
	{
		assert_true((terrain->min_tile_dimension > MIN_TILE_DIMENSION) ||
		            sFloatEquals(terrain->min_tile_dimension, MIN_TILE_DIMENSION));

		// Value before (or equal) 12, dividing 972 by three
		assert_true(sFloatEquals(terrain->min_tile_dimension, 12.0f));

		// Steps at dividing by three: 972, 324, 108, 36, 12
		assert_true(terrain->steps == 5);

		// Previous subdivisions in 2d
		// (972/972)^2 + (972/324)^2 + (972/108)^2 + (972/36)^2 + (972/12)^2
		assert_true(terrain->tiles_no == 7381);

		struct TreeState s = {.start = terrain->root};
		struct Tree* item = NULL;
		struct NTerrainNode* node = NULL;

		while ((item = TreeIterate(&s, &terrain->buffer)) != NULL)
		{
			node = item->data;

			// If tiles are squares, note that I'm using sFloatEquals()
			if (sFloatEquals((node->max.x - node->min.x), (node->max.y - node->min.y)) == false)
				printf("%f != %f\n", (node->max.x - node->min.x), (node->max.y - node->min.y));

			assert_true(sFloatRoughtEquals((node->max.x - node->min.x), (node->max.y - node->min.y)));

			// If dimensions follows the terrain subdivision
			assert_true(sFloatEquals((node->max.x - node->min.x), 972.0f) ||
			            sFloatEquals((node->max.x - node->min.x), 324.0f) ||
			            sFloatEquals((node->max.x - node->min.x), 108.0f) ||
			            sFloatEquals((node->max.x - node->min.x), 36.0f) ||
			            sFloatEquals((node->max.x - node->min.x), 12.0f));
		}
	}

	if (getenv("I_WANT_A_SOUVENIR") != NULL)
		sDrawTerrainLayers(terrain, "TestMesures2.svg");

	NTerrainDelete(terrain);
}


/*-----------------------------

 main()
-----------------------------*/
int main()
{
	const struct CMUnitTest tests[] = {cmocka_unit_test(TestMesures1), cmocka_unit_test(TestMesures2)};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
