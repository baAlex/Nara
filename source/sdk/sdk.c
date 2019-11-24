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

#include "erosion.h"
#include "image.h"
#include "simplex.h"
#include "utilities.h"
#include "vector.h"

#define SEED 123456789
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
static float* sProcessHeightmap(const char* filename, struct Status* st)
{
	float* buffer = NULL;
	struct Image* image = NULL;

	if ((image = ImageCreate(IMAGE_GRAY16, WIDTH, HEIGHT)) == NULL ||
	    (buffer = malloc((sizeof(float)) * WIDTH * HEIGHT)) == NULL)
	{
		StatusSet(st, NULL, STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	float value = 0.0f;
	float min = 0.0f;
	float max = 0.0f;

	float scale = 1000.0f / (float)SCALE;
	float min_dimension = (float)Min(WIDTH, HEIGHT);

	printf("Generating heightmap...\n");

	for (size_t row = 0; row < HEIGHT; row++)
	{
		for (size_t col = 0; col < WIDTH; col++)
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
				min = min;

			buffer[col + WIDTH * row] = value;
		}
	}

	// Normalize
	printf(" - Normalizing...\n");

	for (size_t i = 0; i < (WIDTH * HEIGHT); i++)
	{
		buffer[i] = buffer[i] * (1.0f / max);

		if (buffer[i] < -1.0f)
			buffer[i] = -1.0f;

		if (buffer[i] > 1.0f)
			buffer[i] = 1.0f;
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

		HydraulicErosion(WIDTH, HEIGHT, buffer, &options);
	}

	// Save
	{
		printf(" - Saving '%s'...\n", filename);
		uint16_t* pixel = image->data;

		for (size_t i = 0; i < (WIDTH * HEIGHT); i++)
			pixel[i] = (uint16_t)((buffer[i] + 1.0f) * 32767.5f);

		struct Status temp_st = ImageSaveSgi(image, filename);

		if (temp_st.code != STATUS_SUCCESS)
		{
			if (st != NULL)
				memcpy(st, &temp_st, sizeof(struct Status));

			goto return_failure;
		}
	}

	// Bye!
	ImageDelete(image);
	return buffer;

return_failure:
	if (buffer != NULL)
		free(buffer);
	if (image != NULL)
		ImageDelete(image);
	return NULL;
}


/*-----------------------------

 sProcessNormalmap()
-----------------------------*/
static inline float sHeightAt(const float* hmap, int x, int y)
{
	x = Clamp(x, 0, WIDTH - 1);
	y = Clamp(y, 0, HEIGHT - 1);

	return hmap[x + WIDTH * y];
}

static struct Vector3* sProcessNormalmap(const float* hmap, const char* filename, struct Status* st)
{
	// https://en.wikipedia.org/wiki/Sobel_operator
	// https://en.wikipedia.org/wiki/Image_derivatives
	// https://en.wikipedia.org/wiki/Kernel_(image_processing)#Convolution

	struct Vector3* buffer = NULL;
	struct Image* image = NULL;

	if ((image = ImageCreate(IMAGE_RGB8, WIDTH, HEIGHT)) == NULL ||
	    (buffer = malloc((sizeof(struct Vector3)) * WIDTH * HEIGHT)) == NULL)
	{
		StatusSet(st, NULL, STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	printf("Generating normalmap...\n");

	for (int row = 0; row < HEIGHT; row++)
	{
		for (int col = 0; col < WIDTH; col++)
		{
			buffer[col + WIDTH * row].x =
			    1.0f * (sHeightAt(hmap, col - 1, row - 1) - sHeightAt(hmap, col + 1, row - 1)) +
			    2.0f * (sHeightAt(hmap, col - 1, row) - sHeightAt(hmap, col + 1, row)) +
			    1.0f * (sHeightAt(hmap, col - 1, row + 1) - sHeightAt(hmap, col + 1, row + 1));

			buffer[col + WIDTH * row].y =
			    1.0f * (sHeightAt(hmap, col - 1, row - 1) - sHeightAt(hmap, col - 1, row + 1)) +
			    2.0f * (sHeightAt(hmap, col, row - 1) - sHeightAt(hmap, col, row + 1)) +
			    1.0f * (sHeightAt(hmap, col + 1, row - 1) - sHeightAt(hmap, col + 1, row + 1));

			buffer[col + WIDTH * row].x *= NORMAL_SCALE;
			buffer[col + WIDTH * row].y *= NORMAL_SCALE;
			buffer[col + WIDTH * row].z = 1.0f;

			buffer[col + WIDTH * row] = Vector3Normalize(buffer[col + WIDTH * row]);

			// Slope
			buffer[col + WIDTH * row].z = sqrtf(powf(buffer[col + WIDTH * row].x, 2.0f) + powf(buffer[col + WIDTH * row].y, 2.0));
		}
	}

	// Save
	{
		printf(" - Saving '%s'...\n", filename);

		struct
		{
			uint8_t r, g, b
		}* pixel = image->data;

		for (size_t i = 0; i < (WIDTH * HEIGHT); i++)
		{
			pixel[i].r = (uint8_t)floorf((buffer[i].x + 1.0f) * 128.0f);
			pixel[i].g = (uint8_t)floorf((buffer[i].y + 1.0f) * 128.0f);
			pixel[i].b = (uint8_t)floorf((buffer[i].z + 1.0f) * 128.0f);
		}

		struct Status temp_st = ImageSaveSgi(image, filename);

		if (temp_st.code != STATUS_SUCCESS)
		{
			if (st != NULL)
				memcpy(st, &temp_st, sizeof(struct Status));

			goto return_failure;
		}
	}

	// Bye!
	ImageDelete(image);
	return buffer;

return_failure:
	if (buffer != NULL)
		free(buffer);
	if (image != NULL)
		ImageDelete(image);
	return NULL;
}


/*-----------------------------

 main()
-----------------------------*/
int main()
{
	struct Status st = {0};

	float* heightmap = NULL;
	struct Vector3* normalmap = NULL;

	SimplexSeed(&s_perm_a, SEED);
	SimplexSeed(&s_perm_b, SEED + 1);
	SimplexSeed(&s_perm_c, SEED + 2);
	SimplexSeed(&s_perm_d, SEED + 3);
	SimplexSeed(&s_perm_e, SEED + 4);

	if ((heightmap = sProcessHeightmap("heightmap.sgi", &st)) == NULL)
		goto return_failure;

	if ((normalmap = sProcessNormalmap(heightmap, "normalmap.sgi", &st)) == NULL)
		goto return_failure;

	// Bye!
	free(heightmap);
	free(normalmap);
	return EXIT_SUCCESS;

return_failure:
	StatusPrint("Nara Sdk", st);

	if (heightmap != NULL)
		free(heightmap);
	if (normalmap != NULL)
		free(normalmap);
	return EXIT_FAILURE;
}
