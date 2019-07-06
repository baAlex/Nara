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

 [terrain.c]
 - Alexander Brandt 2019
-----------------------------*/

#include "terrain.h"
#include "image.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static struct Vector4 s_colors[] = {{1.0, 0.5, 0.5, 1.0}, {0.5, 1.0, 0.5, 1.0}, {0.5, 0.5, 1.0, 1.0},
									{1.0, 0.0, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}};


static inline float sPixelAsFloat(const struct Image* image, int x, int y)
{
	uint8_t v = 0;

	switch (image->format)
	{
	case IMAGE_GRAY8:
		v = ((uint8_t*)image->data)[x + image->width * y];
		return (float)v / 255.0;
	default:
		break;
	}

	return 0.0;
}


/*-----------------------------

 TerrainLoad()
-----------------------------*/
int TerrainLoad(struct Terrain* terrain, int width, int height, const char* heightmap_filename, struct Status* st)
{
	struct Image* heightmap_image = NULL;
	int heightmap_step_x = 0;
	int heightmap_step_y = 0;

	union {
		struct Vertex* v;
		uint16_t* i;
		void* raw;

	} temp;

	TerrainClean(terrain);
	StatusSet(st, "TerrainLoad", STATUS_SUCCESS, NULL);

	temp.raw = NULL;

	if (heightmap_filename != NULL)
	{
		if ((heightmap_image = ImageLoad(heightmap_filename, st)) == NULL)
			goto return_failure;
	}

	if ((temp.raw = malloc(sizeof(struct Vertex) * (width + 1) * (height + 1))) == NULL)
	{
		StatusSet(st, "TerrainLoad", STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	terrain->width = width;
	terrain->height = height;

	// Vertices
	for (int r = 0; r < (terrain->height + 1); r++)
	{
		heightmap_step_x = 0;
		for (int c = 0; c < (terrain->width + 1); c++)
		{
			temp.v[c + (terrain->width + 1) * r].col = s_colors[rand() % (sizeof(s_colors) / sizeof(struct Vector4))];

			temp.v[c + (terrain->width + 1) * r].pos.x = (float)c;
			temp.v[c + (terrain->width + 1) * r].pos.y = (float)r;
			temp.v[c + (terrain->width + 1) * r].pos.z = 0.0;

			if (heightmap_image != NULL)
			{
				temp.v[c + (terrain->width + 1) * r].pos.z =
					sPixelAsFloat(heightmap_image, heightmap_step_x, heightmap_step_y) * 5.0;

				if (heightmap_image != NULL && c < terrain->width)
					heightmap_step_x += heightmap_image->width / terrain->width;
			}
		}

		if (heightmap_image != NULL && r < terrain->height)
			heightmap_step_y += heightmap_image->height / terrain->height;
	}

	if ((terrain->vertices = VerticesCreate(temp.v, (width + 1) * (height + 1), st)) == NULL)
		goto return_failure;

	// Index
	size_t index_i = 0; // Index index

	for (int r = 0; r < (terrain->height); r++)
	{
		for (int c = 0; c < (terrain->width); c++)
		{
			temp.i[index_i + 0] = c + ((terrain->width + 1) * r);
			temp.i[index_i + 1] = c + ((terrain->width + 1) * r) + 1;
			temp.i[index_i + 2] = c + ((terrain->width + 1) * r) + 1 + (terrain->width + 1);
			temp.i[index_i + 3] = c + ((terrain->width + 1) * r) + 1 + (terrain->width + 1);
			temp.i[index_i + 4] = c + ((terrain->width + 1) * r) + (terrain->width + 1);
			temp.i[index_i + 5] = c + ((terrain->width + 1) * r);
			index_i += 6;
		}
	}

	if ((terrain->index = IndexCreate(temp.i, (terrain->width * terrain->height * 6), st)) == NULL)
		goto return_failure;

	// Bye!
	free(temp.raw);
	if (heightmap_image != NULL)
		ImageDelete(heightmap_image);

	return 0;

return_failure:
	if (heightmap_image != NULL)
		ImageDelete(heightmap_image);
	if (temp.raw != NULL)
		free(temp.raw);

	if (terrain->vertices != 0)
		VerticesDelete(terrain->vertices);
	if (terrain->index != 0)
		IndexDelete(terrain->index);

	return 1;
}


/*-----------------------------

 TerrainDelete()
-----------------------------*/
void TerrainClean(struct Terrain* t)
{
	if (t->index != 0)
		IndexDelete(t->index);

	if (t->vertices != 0)
		VerticesDelete(t->vertices);

	memset(t, 0, sizeof(struct Terrain));
}


/*-----------------------------

 TerrainGetVertices()
-----------------------------*/
inline struct Vertices* TerrainGetVertices(struct Terrain* terrain) { return terrain->vertices; }


/*-----------------------------

 TerrainGetIndex()
-----------------------------*/
inline struct Index* TerrainGetIndex(struct Terrain* terrain, int lod_level, struct Vector3 area_start,
									 struct Vector3 area_end)
{
	(void)lod_level;  // TODO: Not today, but in the future...
	(void)area_start; // The idea is to have many indexes stored in the
	(void)area_end;   // terrain struct, returning them depending on
					  // desired lod and position. Kind of quadtree.

	return terrain->index;
}
