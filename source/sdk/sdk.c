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

#include "erosion.h"
#include "image.h"
#include "simplex.h"
#include "utilities.h"

#define SEED 123456789
#define WIDTH 1024
#define HEIGHT 1024
#define SCALE 200

static struct Permutations s_perm_a;
static struct Permutations s_perm_b;
static struct Permutations s_perm_c;
static struct Permutations s_perm_d;
static struct Permutations s_perm_e;


int main()
{
	struct Status st = {0};
	float* buffer = NULL;
	struct Image* image = NULL;

	SimplexSeed(&s_perm_a, SEED);
	SimplexSeed(&s_perm_b, SEED + 1);
	SimplexSeed(&s_perm_c, SEED + 2);
	SimplexSeed(&s_perm_d, SEED + 3);
	SimplexSeed(&s_perm_e, SEED + 4);

	if ((image = ImageCreate(IMAGE_GRAY16, WIDTH, HEIGHT)) == NULL ||
	    (buffer = malloc((sizeof(float)) * WIDTH * HEIGHT)) == NULL)
	{
		StatusSet(&st, NULL, STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	float value = 0.0f;
	float min = 0.0f;
	float max = 0.0f;

	float scale = 1000.0f / (float)SCALE;
	float min_dimension = (float)Min(image->width, image->height);

	// Generate heightmap
	printf("Generating heightmap...\n");

	for (size_t row = 0; row < image->height; row++)
	{
		for (size_t col = 0; col < image->width; col++)
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

			buffer[col + image->width * row] = value;
		}
	}

	// Normalize heightmap
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

	// Save heightmap
	{
		printf("Saving heightmap...\n");
		uint16_t* pixel = image->data;

		for (size_t i = 0; i < (WIDTH * HEIGHT); i++)
			pixel[i] = (uint16_t)((buffer[i] + 1.0f) * 32767.5f);

		st = ImageSaveSgi(image, "heightmap.sgi");

		if (st.code != STATUS_SUCCESS)
			goto return_failure;
	}

	// Bye!
	free(buffer);
	ImageDelete(image);

	return EXIT_SUCCESS;

return_failure:
	if (buffer != NULL)
		free(buffer);
	if (image != NULL)
		ImageDelete(image);

	StatusPrint("Nara Sdk", st);
	return EXIT_FAILURE;
}
