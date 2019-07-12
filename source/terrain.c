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

#include <math.h>
#include <stdlib.h>
#include <string.h>


/*-----------------------------

 sPixelAsFloat()
-----------------------------*/
static inline float sPixelAsFloat(const struct Image* image, float x, float y)
{
	uint8_t v = 0;

	switch (image->format)
	{
	case IMAGE_GRAY8:
		v = ((uint8_t*)image->data)[(int)floor(x) + image->width * (int)floor(y)];
		return (float)v / 255.0;
	default:
		break;
	}

	return 0.0;
}


/*-----------------------------

 TerrainCreate()
-----------------------------*/
struct Terrain* TerrainCreate(struct TerrainOptions options, struct Status* st)
{
	struct Terrain* terrain = NULL;
	float img_step_x = 0;
	float img_step_y = 0;

	union {
		struct Vertex* v;
		uint16_t* i;
		void* raw;

	} temp;

	temp.raw = NULL;
	StatusSet(st, "TerrainCreate", STATUS_SUCCESS, NULL);

	if ((terrain = calloc(1, sizeof(struct Terrain))) == NULL)
		goto return_failure;

	memcpy(&terrain->options, &options, sizeof(struct TerrainOptions));

	if (options.heightmap_filename != NULL)
	{
		if ((terrain->heightmap = ImageLoad(options.heightmap_filename, st)) == NULL)
			goto return_failure;
	}

	// Vertices
	if ((temp.raw = malloc(sizeof(struct Vertex) * (options.width + 1) * (options.height + 1))) == NULL)
	{
		StatusSet(st, "TerrainCreate", STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	for (size_t r = 0; r < (options.height + 1); r++)
	{
		for (size_t c = 0; c < (options.width + 1); c++)
		{
			temp.v[c + (options.width + 1) * r].col = (struct Vector4){1.0, 1.0, 1.0, 1.0};

			temp.v[c + (options.width + 1) * r].pos.x = (float)c;
			temp.v[c + (options.width + 1) * r].pos.y = (float)r;
			temp.v[c + (options.width + 1) * r].pos.z = 0.0;

			if (terrain->heightmap != NULL)
			{
				temp.v[c + (options.width + 1) * r].pos.z =
					sPixelAsFloat(terrain->heightmap, img_step_x, img_step_y) * (float)options.elevation;

				if (c < options.width)
					img_step_x += (float)terrain->heightmap->width / (float)options.width;
			}
		}

		if (terrain->heightmap != NULL && r < options.height)
		{
			img_step_y += (float)terrain->heightmap->height / (float)options.height;
			img_step_x = 0;
		}
	}

	if ((terrain->vertices = VerticesCreate(temp.v, (options.width + 1) * (options.height + 1), st)) == NULL)
		goto return_failure;

	// Index
	size_t index_i = 0; // Index index

	for (size_t r = 0; r < (options.height); r++)
	{
		for (size_t c = 0; c < (options.width); c++)
		{
			temp.i[index_i + 0] = c + ((options.width + 1) * r);
			temp.i[index_i + 1] = c + ((options.width + 1) * r) + 1;
			temp.i[index_i + 2] = c + ((options.width + 1) * r) + 1 + (options.width + 1);
			temp.i[index_i + 3] = c + ((options.width + 1) * r) + 1 + (options.width + 1);
			temp.i[index_i + 4] = c + ((options.width + 1) * r) + (options.width + 1);
			temp.i[index_i + 5] = c + ((options.width + 1) * r);
			index_i += 6;
		}
	}

	if ((terrain->index = IndexCreate(temp.i, (options.width * options.height * 6), st)) == NULL)
		goto return_failure;

	// Bye!
	free(temp.raw);
	return terrain;

return_failure:
	if (temp.raw != NULL)
		free(temp.raw);
	if (terrain != NULL)
		TerrainDelete(terrain);

	return NULL;
}


/*-----------------------------

 TerrainDelete()
-----------------------------*/
void TerrainDelete(struct Terrain* terrain)
{
	if (terrain->index != NULL)
		IndexDelete(terrain->index);

	if (terrain->vertices != NULL)
		VerticesDelete(terrain->vertices);

	if (terrain->heightmap != NULL)
		ImageDelete(terrain->heightmap);

	// if (terrain->color != NULL)
	// TextureDelete(terrain->color);

	free(terrain);
}
