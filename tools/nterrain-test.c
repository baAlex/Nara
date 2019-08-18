/*-----------------------------

 [nterrain-test.c]
 - Alexander Brandt 2019

 $ clang -I./source/lib-japan/include -I./source/engine/ ./source/lib-japan/source/buffer.c
./source/lib-japan/source/endianness.c ./source/lib-japan/source/image.c ./source/lib-japan/source/image-sgi.c
./source/lib-japan/source/status.c ./source/lib-japan/source/tree.c ./source/lib-japan/source/vector.c
./source/engine/nterrain.c ./tools/nterrain-test.c -lm -lcmocka -O0 -g -o ./test

-----------------------------*/

#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <cmocka.h>

#include "image.h"
#include "nterrain.h"

#define BORDER 10


struct Canvas
{
	struct Vector2i offset;
	struct Vector3 color;
	struct Image* image;
};


/*-----------------------------

 sDrawDot()
-----------------------------*/
static void sDrawDot(struct Canvas* canvas, struct Vector2i pos)
{
	uint8_t* pixel = canvas->image->data;

	pixel[(pos.x + canvas->offset.x + canvas->image->width * (pos.y + canvas->offset.y)) * 3 + 0] =
	    canvas->color.x * 255;
	pixel[(pos.x + canvas->offset.x + canvas->image->width * (pos.y + canvas->offset.y)) * 3 + 1] =
	    canvas->color.y * 255;
	pixel[(pos.x + canvas->offset.x + canvas->image->width * (pos.y + canvas->offset.y)) * 3 + 2] =
	    canvas->color.z * 255;
}


/*-----------------------------

 sDrawLine()
-----------------------------*/
static void sDrawLine(struct Canvas* canvas, struct Vector2i a, struct Vector2i b)
{
	// https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
	int dx = abs(b.x - a.x);
	int sx = (a.x < b.x) ? 1 : -1;
	int dy = -abs(b.y - a.y);
	int sy = (a.y < b.y) ? 1 : -1;
	int err = dx + dy; // error value e_xy
	int e2;

	while (a.x != b.x || a.y != b.y)
	{
		sDrawDot(canvas, a);

		e2 = 2 * err;

		if (e2 >= dy)
		{
			err += dy;
			a.x += sx;
		}

		if (e2 <= dx)
		{
			err += dx;
			a.y += sy;
		}
	}
}


/*-----------------------------

 sInitCanvas()
-----------------------------*/
static int sInitCanvas(struct Canvas* canvas, size_t width, size_t height)
{
	if ((canvas->image = ImageCreate(IMAGE_RGB8, width, height)) == NULL)
		return 1;

	memset(canvas->image->data, 0, canvas->image->size);
	canvas->offset = (struct Vector2i){0, 0};
	canvas->color = (struct Vector3){1.0, 1.0, 1.0};
	return 0;
}


/*-----------------------------

 sDrawTerrain()
-----------------------------*/
static void sDrawTerrain(struct NTerrain* terrain, struct Canvas* canvas)
{
	struct TreeState s = {.start = terrain->root};
	struct Buffer buffer = {0};
	struct Tree* item = NULL;
	struct NTerrainNode* node = NULL;

	size_t prev_depth = 0;
	size_t index1 = 0;
	size_t index2 = 0;
	size_t index3 = 0;

	while ((item = TreeIterate(&s, &buffer)) != NULL)
	{
		node = item->data;

		switch (node->vertices_type)
		{
		case INHERITED_FROM_PARENT: canvas->color = (struct Vector3){0.5, 0.5, 0.5}; break;
		case SHARED_WITH_CHILDRENS: canvas->color = (struct Vector3){0.0, 0.5, 0.0}; break;
		case OWN_VERTICES: canvas->color = (struct Vector3){0.5, 0.0, 0.0}; break;
		}

		canvas->offset = (struct Vector2i){BORDER, BORDER + (BORDER + terrain->dimension) * s.depth};

		for (size_t i = 0; i < node->index_no; i += 3)
		{
			index1 = node->index[i];
			index2 = node->index[i + 1];
			index3 = node->index[i + 2];

			sDrawLine(canvas, (struct Vector2i){node->vertices[index1].pos.x, node->vertices[index1].pos.y},
			          (struct Vector2i){node->vertices[index2].pos.x, node->vertices[index2].pos.y});

			sDrawLine(canvas, (struct Vector2i){node->vertices[index2].pos.x, node->vertices[index2].pos.y},
			          (struct Vector2i){node->vertices[index3].pos.x, node->vertices[index3].pos.y});

			sDrawLine(canvas, (struct Vector2i){node->vertices[index3].pos.x, node->vertices[index3].pos.y},
			          (struct Vector2i){node->vertices[index1].pos.x, node->vertices[index1].pos.y});
		}

		canvas->color = Vector3Scale(canvas->color, 2);

		sDrawLine(canvas, (struct Vector2i){node->min.x, node->min.y}, (struct Vector2i){node->min.x, node->max.y});
		sDrawLine(canvas, (struct Vector2i){node->min.x, node->max.y}, (struct Vector2i){node->max.x, node->max.y});
		sDrawLine(canvas, (struct Vector2i){node->max.x, node->max.y}, (struct Vector2i){node->max.x, node->min.y});
		sDrawLine(canvas, (struct Vector2i){node->max.x, node->min.y}, (struct Vector2i){node->min.x, node->min.y});
	}
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
	struct Status st = {0};
	struct Canvas c = {0};

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

	#ifndef NO_SGI
	if (sInitCanvas(&c, terrain->dimension + BORDER * 2, BORDER + (terrain->dimension + BORDER) * terrain->steps) == 0)
	{
		sDrawTerrain(terrain, &c);
		ImageSaveSgi(c.image, "./TestNormalTerrain.sgi");
		ImageDelete(c.image);
	}
	#endif

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
// fixed them. But I still are worried about the loss of precision.

// Put an eye on the mallocs operations number that Valgrind reports. From
// an normal of ~700 it jumps to ~7000. In other cases it simply stall the heap.
#define TERRAIN_DIMENSION 972.0
#define TILE_DIMENSION 12.0
#define PATTERN_SUBDIVISION 3

	struct NTerrain* terrain = NULL;
	struct Status st = {0};
	struct Canvas c = {0};

	if ((terrain = NTerrainCreate(TERRAIN_DIMENSION, TILE_DIMENSION, PATTERN_SUBDIVISION, &st)) == NULL)
	{
		StatusPrint("TestPreciseTerrain", st);
		assert_int_equal(st.code, STATUS_SUCCESS);
	}

	sPrintInfo(terrain);

	#ifndef NO_SGI
	if (sInitCanvas(&c, terrain->dimension + BORDER * 2, BORDER + (terrain->dimension + BORDER) * terrain->steps) == 0)
	{
		sDrawTerrain(terrain, &c);
		ImageSaveSgi(c.image, "./TestPreciseTerrain.sgi");
		ImageDelete(c.image);
	}
	#endif

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
	const struct CMUnitTest tests[] = {cmocka_unit_test(TestNormalTerrain), cmocka_unit_test(TestPreciseTerrain)};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
