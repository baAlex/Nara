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

#include "buffer.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>


/*-----------------------------

 sPixelAsFloat()
-----------------------------*/
static inline float sPixelAsFloat(const struct Image* image, float x, float y)
{
	uint8_t v8 = 0;
	uint16_t v16 = 0;

	switch (image->format)
	{
	case IMAGE_GRAY8:
		v8 = ((uint8_t*)image->data)[(int)floor(x) + image->width * (int)floor(y)];
		return (float)v8 / 255.0;
	case IMAGE_GRAY16:
		v16 = ((uint16_t*)image->data)[(int)floor(x) + image->width * (int)floor(y)];
		return (float)v16 / 65535.0;
	default:
		break;
	}

	return 0.0;
}


/*-----------------------------

 sGenerateVertices()
-----------------------------*/
static struct Vertices* sGenerateVertices(struct Buffer* buffer, const struct Image* heightmap,
                                          struct Vector2i terrain_size, struct Vector2i tile_size, int elevation,
                                          struct Status* st)
{
	struct Vertices* vertices = NULL;
	struct Vertex* temp_vertices = NULL;

	int horizontal_tiles = 0;
	int vertical_tiles = 0;

	float heightmap_step_x = 0;
	float heightmap_step_y = 0;

	StatusSet(st, "GenerateVertices", STATUS_SUCCESS, NULL);

	// Allocate memory
	if ((terrain_size.x % tile_size.x) != 0 || (terrain_size.y % tile_size.y) != 0)
	{
		StatusSet(st, "GenerateVertices", STATUS_ERROR,
		          "Terrain dimensions %ix%i not integer divisible by tile dimensions %ix%i", terrain_size.x,
		          terrain_size.y, tile_size.x, tile_size.y);
		return NULL;
	}

	horizontal_tiles = terrain_size.x / tile_size.x;
	vertical_tiles = terrain_size.y / tile_size.y;

	if ((horizontal_tiles * vertical_tiles) >= UINT16_MAX) // Because indices in OpenGL ES2
	{
		StatusSet(st, "GenerateVertices", STATUS_ERROR,
		          "Number of vertices exceds 2^16, tile dimensions %ix%i too tiny", tile_size.x, tile_size.y);
		return NULL;
	}

	if (BufferResize(buffer, sizeof(struct Vertex) * (horizontal_tiles + 1) * (vertical_tiles + 1)) == NULL)
	{
		StatusSet(st, "GenerateVertices", STATUS_MEMORY_ERROR, NULL);
		return NULL;
	}

	temp_vertices = buffer->data;

	// Generate vertices
	for (int row = 0; row < (vertical_tiles + 1); row++)
	{
		for (int col = 0; col < (horizontal_tiles + 1); col++)
		{
			temp_vertices[col + (horizontal_tiles + 1) * row].uv.x = heightmap_step_x;
			temp_vertices[col + (horizontal_tiles + 1) * row].uv.y = heightmap_step_y;

			temp_vertices[col + (horizontal_tiles + 1) * row].pos.x = (float)(col * tile_size.x);
			temp_vertices[col + (horizontal_tiles + 1) * row].pos.y = (float)(row * tile_size.y);

			if (heightmap != NULL)
			{
				temp_vertices[col + (horizontal_tiles + 1) * row].pos.z =
					sPixelAsFloat(heightmap, (float)heightmap->width * heightmap_step_x,
								  (float)heightmap->height * heightmap_step_y) *
					(float)elevation;
			}
			else
			{
				temp_vertices[col + (horizontal_tiles + 1) * row].pos.z = 0.0;
			}

			heightmap_step_x += (float)tile_size.x / (float)terrain_size.x;
		}

		heightmap_step_y += (float)tile_size.y / (float)terrain_size.y;
		heightmap_step_x = 0.0;
	}

	if ((vertices = VerticesCreate(temp_vertices, (horizontal_tiles + 1) * (vertical_tiles + 1), st)) == NULL)
		return NULL;

	return vertices;
}


/*-----------------------------

 sGenerateIndex()
-----------------------------*/
static struct Index* sGenerateIndex(struct Buffer* buffer, struct Vector2i terrain_size, struct Vector2i tile_size,
									struct Status* st)
{
	struct Index* index = NULL;
	uint16_t* temp_index = NULL;

	int horizontal_tiles = terrain_size.x / tile_size.x;
	int vertical_tiles = terrain_size.y / tile_size.y;

	// Allocate memory
	StatusSet(st, "GenerateIndex", STATUS_SUCCESS, NULL);

	if (BufferResize(buffer, sizeof(uint16_t) * (horizontal_tiles * vertical_tiles * 6)) == NULL)
	{
		StatusSet(st, "GenerateIndex", STATUS_MEMORY_ERROR, NULL);
		return NULL;
	}

	temp_index = buffer->data;

	// Generate index
	size_t index_i = 0; // Index index

	for (int row = 0; row < vertical_tiles + 0; row++)
	{
		for (int col = 0; col < horizontal_tiles + 0; col++)
		{
			// TODO: diagonal pattern when drawing as GL_LINES

			#if 1
			temp_index[index_i + 0] = col + ((horizontal_tiles + 1) * row);
			temp_index[index_i + 1] = col + ((horizontal_tiles + 1) * row) + 1;
			temp_index[index_i + 2] = col + ((horizontal_tiles + 1) * row) + 1 + (horizontal_tiles + 1);
			#else
			temp_index[index_i + 0] = 0;
			temp_index[index_i + 1] = 0;
			temp_index[index_i + 2] = 0;
			#endif

			#if 1
			temp_index[index_i + 3] = col + ((horizontal_tiles + 1) * row);
			temp_index[index_i + 4] = col + ((horizontal_tiles + 1) * row) + 1 + (horizontal_tiles + 1);
			temp_index[index_i + 5] = col + ((horizontal_tiles + 1) * row) + (horizontal_tiles + 1);
			#else
			temp_index[index_i + 3] = 0;
			temp_index[index_i + 4] = 0;
			temp_index[index_i + 5] = 0;
			#endif

			index_i += 6;
		}
	}

	if ((index = IndexCreate(temp_index, (horizontal_tiles * vertical_tiles * 6), st)) == NULL)
		return NULL;

	return index;
}


/*-----------------------------

 TerrainCreate()
-----------------------------*/
struct Terrain* TerrainCreate(struct TerrainOptions options, struct Status* st)
{
	struct Terrain* terrain = NULL;
	struct Image* colormap_image = NULL;
	struct Buffer buffer = {0};

	StatusSet(st, "TerrainCreate", STATUS_SUCCESS, NULL);

	if ((terrain = calloc(1, sizeof(struct Terrain))) == NULL)
		goto return_failure;

	memcpy(&terrain->options, &options, sizeof(struct TerrainOptions));

	// Images
	if (options.heightmap_filename != NULL)
	{
		if ((terrain->heightmap = ImageLoad(options.heightmap_filename, st)) == NULL)
			goto return_failure;
	}

	if (options.colormap_filename != NULL)
	{
		if ((colormap_image = ImageLoad(options.colormap_filename, st)) == NULL)
			goto return_failure;
	}

	// Vertices-Index
	struct Vector2i map_dimensions = {options.width, options.height};
	struct Vector2i tile_dimensions = {8, 8};

	if ((terrain->vertices = sGenerateVertices(&buffer, terrain->heightmap, map_dimensions, tile_dimensions,
											   options.elevation, st)) == NULL)
		goto return_failure;

	if ((terrain->index = sGenerateIndex(&buffer, map_dimensions, tile_dimensions, st)) == NULL)
		goto return_failure;

	// Color texture
	if (options.colormap_filename != NULL)
	{
		if ((terrain->color = TextureCreate(colormap_image, st)) == NULL)
			goto return_failure;

		ImageDelete(colormap_image);
	}

	// Bye!
	BufferClean(&buffer);
	return terrain;

return_failure:
	BufferClean(&buffer);
	if (colormap_image != NULL)
		ImageDelete(colormap_image);
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

	if (terrain->color != NULL)
		TextureDelete(terrain->color);

	free(terrain);
}
