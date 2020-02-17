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

 [sdk.c]
 - Alexander Brandt 2019
-----------------------------*/

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "japan-image.h"
#include "japan-utilities.h"
#include "japan-vector.h"

#include "erosion.h"
#include "simplex.h"

#define SEED 666
#define WIDTH 1024
#define HEIGHT 1024
#define SCALE 200
#define NORMAL_SCALE 2

static struct Permutations s_perm_a;
static struct Permutations s_perm_b;
static struct Permutations s_perm_c;
static struct Permutations s_perm_d;
static struct Permutations s_perm_e;


/*-----------------------------

 sProcessHeightmap()
-----------------------------*/
static float* sProcessHeightmap(int width, int height, const char* filename, struct jaStatus* st)
{
	float* buffer = NULL;
	struct jaImage* image = NULL;

	if ((image = jaImageCreate(IMAGE_U16, (size_t)width, (size_t)height, 1)) == NULL ||
	    (buffer = malloc((sizeof(float)) * (size_t)width * (size_t)height)) == NULL)
	{
		jaStatusSet(st, NULL, STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	float value = 0.0f;
	float min = 0.0f;
	float max = 0.0f;

	float scale = 1000.0f / (float)SCALE;
	float min_dimension = (float)jaMin(width, height);

	printf("Generating heightmap...\n");

	for (int row = 0; row < height; row++)
	{
		for (int col = 0; col < width; col++)
		{
			float x = ((float)row / min_dimension) * scale;
			float y = ((float)col / min_dimension) * scale;

			value = Simplex2d(&s_perm_a, x, y);
			value += Simplex2d(&s_perm_b, x * 2.0f, y * 2.0f) / 2.0f;
			value += Simplex2d(&s_perm_c, x * 4.0f, y * 4.0f) / 4.0f;
			value += Simplex2d(&s_perm_d, x * 8.0f, y * 8.0f) / 8.0f;
			value += Simplex2d(&s_perm_e, x * 16.0f, y * 16.0f) / 16.0f;
			value *= 1.0f / (1.0f + (1.0f / 2.0f) + (1.0f / 4.0f) + (1.0f / 8.0f) + (1.0f / 16.0f));

			if (value > max)
				max = value;

			if (value < min)
				min = value;

			buffer[col + width * row] = value;
		}
	}

	// Normalize
	printf(" - Normalizing...\n");

	for (int i = 0; i < (width * height); i++)
	{
		buffer[i] = buffer[i] * (1.0f / max);
		buffer[i] = jaClamp(buffer[i], -1.0f, 1.0f);
	}

	// Simulate hydraulic erosion
	{
		printf(" - Simulating hydraulic erosion...\n");

		struct ErodeOptions options = {0};
		options.n = 50000;
		options.ttl = 30;
		options.p_radius = 4;
		options.p_enertia = 0.1f;
		options.p_capacity = 50.0f;
		options.p_gravity = 4.0f;
		options.p_evaporation = 0.1f;
		options.p_erosion = 0.1f;
		options.p_deposition = 1.0f;
		options.p_min_slope = 0.1f;

		HydraulicErosion(width, height, buffer, &options);
	}

	// Save
	{
		printf(" - Saving '%s'...\n", filename);
		uint16_t* pixel = image->data;

		for (int i = 0; i < (width * height); i++)
			pixel[i] = (uint16_t)((buffer[i] + 1.0f) * 32767.5f);

		if (jaImageSaveSgi(image, filename, st) != 0)
			goto return_failure;
	}

	// Bye!
	jaImageDelete(image);
	return buffer;

return_failure:
	if (buffer != NULL)
		free(buffer);
	if (image != NULL)
		jaImageDelete(image);
	return NULL;
}


/*-----------------------------

 sProcessNormalmap()
-----------------------------*/
static inline float sHeightAt(const float* hmap, int width, int height, int x, int y)
{
	x = jaClamp(x, 0, width - 1);
	y = jaClamp(y, 0, height - 1);

	return hmap[x + width * y];
}

static struct jaVector3* sProcessNormalmap(int width, int height, const float* hmap, const char* filename,
                                           struct jaStatus* st)
{
	// https://en.wikipedia.org/wiki/Sobel_operator
	// https://en.wikipedia.org/wiki/Image_derivatives
	// https://en.wikipedia.org/wiki/Kernel_(image_processing)#Convolution

	struct jaVector3* buffer = NULL;
	struct jaImage* image = NULL;

	if ((image = jaImageCreate(IMAGE_U8, (size_t)width, (size_t)height, 3)) == NULL ||
	    (buffer = malloc((sizeof(struct jaVector3)) * (size_t)width * (size_t)height)) == NULL)
	{
		jaStatusSet(st, NULL, STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	printf("Generating normalmap...\n");

	for (int row = 0; row < height; row++)
	{
		for (int col = 0; col < width; col++)
		{
			buffer[col + width * row].x =
			    1.0f * (sHeightAt(hmap, width, height, col - 1, row - 1) - sHeightAt(hmap, width, height, col + 1, row - 1)) +
			    2.0f * (sHeightAt(hmap, width, height, col - 1, row) - sHeightAt(hmap, width, height, col + 1, row)) +
			    1.0f * (sHeightAt(hmap, width, height, col - 1, row + 1) - sHeightAt(hmap, width, height, col + 1, row + 1));

			buffer[col + width * row].y =
			    1.0f * (sHeightAt(hmap, width, height, col - 1, row - 1) - sHeightAt(hmap, width, height, col - 1, row + 1)) +
			    2.0f * (sHeightAt(hmap, width, height, col, row - 1) - sHeightAt(hmap, width, height, col, row + 1)) +
			    1.0f * (sHeightAt(hmap, width, height, col + 1, row - 1) - sHeightAt(hmap, width, height, col + 1, row + 1));

			buffer[col + width * row].x *= NORMAL_SCALE;
			buffer[col + width * row].y *= NORMAL_SCALE;
			buffer[col + width * row].z = 1.0f;

			buffer[col + width * row] = jaVector3Normalize(buffer[col + width * row]);

			// Slope
			buffer[col + width * row].z =
			    sqrtf(powf(buffer[col + width * row].x, 2.0f) + powf(buffer[col + width * row].y, 2.0));
		}
	}

	// Save
	{
		printf(" - Saving '%s'...\n", filename);

		struct
		{
			uint8_t r, g, b;
		}* pixel = image->data;

		for (int i = 0; i < (width * height); i++)
		{
			pixel[i].r = (uint8_t)floorf((buffer[i].x + 1.0f) * 128.0f);
			pixel[i].g = (uint8_t)floorf((buffer[i].y + 1.0f) * 128.0f);
			pixel[i].b = (uint8_t)floorf((buffer[i].z + 1.0f) * 128.0f);
		}

		if (jaImageSaveSgi(image, filename, st) != 0)
			goto return_failure;
	}

	// Bye!
	jaImageDelete(image);
	return buffer;

return_failure:
	if (buffer != NULL)
		free(buffer);
	if (image != NULL)
		jaImageDelete(image);
	return NULL;
}


/*-----------------------------

 main()
-----------------------------*/
int main(int argc, const char* argv[])
{
	struct jaStatus st = {0};

	float* heightmap = NULL;
	int heightmap_width = 0;
	int heightmap_height = 0;

	struct jaVector3* normalmap = NULL;

	if (argc <= 1)
	{
		SimplexSeed(&s_perm_a, SEED);
		SimplexSeed(&s_perm_b, SEED + 1);
		SimplexSeed(&s_perm_c, SEED + 2);
		SimplexSeed(&s_perm_d, SEED + 3);
		SimplexSeed(&s_perm_e, SEED + 4);

		if ((heightmap = sProcessHeightmap(WIDTH, HEIGHT, "heightmap.sgi", &st)) == NULL)
			goto return_failure;

		heightmap_width = WIDTH;
		heightmap_height = HEIGHT;
	}
	else
	{
		struct jaImage* image = NULL;
		printf("Loading heightmap '%s'...\n", argv[1]);

		if ((image = jaImageLoad(argv[1], &st)) == NULL)
			goto return_failure;

		if ((heightmap = malloc(image->width * image->height * sizeof(float))) == NULL)
		{
			jaStatusSet(&st, "main", STATUS_MEMORY_ERROR, NULL);
			goto return_failure;
		}

		if (image->format == IMAGE_U16)
		{
			for (size_t i = 0; i < (image->width * image->height); i++)
				heightmap[i] = (float)(((uint16_t*)image->data)[i]) / 65536.0f;
		}
		else if (image->format == IMAGE_U8)
		{
			for (size_t i = 0; i < (image->width * image->height); i++)
				heightmap[i] = (float)(((uint8_t*)image->data)[i]) / 255.0f;
		}
		else
		{
			jaStatusSet(&st, "main", STATUS_UNKNOWN_FILE_FORMAT, "Only grayscale images supported as heightmaps");
			goto return_failure;
		}

		heightmap_width = (int)image->width;
		heightmap_height = (int)image->height;

		jaImageDelete(image);
	}

	if ((normalmap = sProcessNormalmap(heightmap_width, heightmap_height, heightmap, "normalmap.sgi", &st)) == NULL)
		goto return_failure;

	// Bye!
	free(heightmap);
	free(normalmap);
	return EXIT_SUCCESS;

return_failure:
	jaStatusPrint("Nara Sdk", st);

	if (heightmap != NULL)
		free(heightmap);
	if (normalmap != NULL)
		free(normalmap);
	return EXIT_FAILURE;
}
