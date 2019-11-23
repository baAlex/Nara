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

#define SEED 123456789
#define WIDTH 1024
#define HEIGHT 1024
#define SCALE 200

static struct Permutations s_perm_a;
static struct Permutations s_perm_b;
static struct Permutations s_perm_c;
static struct Permutations s_perm_d;
static struct Permutations s_perm_e;


static inline double sMin(double a, double b)
{
	return (a < b) ? a : b;
}

int main()
{
	struct Status st = {0};
	double* buffer = NULL;
	struct Image* image = NULL;

	SimplexSeed(&s_perm_a, SEED);
	SimplexSeed(&s_perm_b, SEED + 1);
	SimplexSeed(&s_perm_c, SEED + 2);
	SimplexSeed(&s_perm_d, SEED + 3);
	SimplexSeed(&s_perm_e, SEED + 4);

	if ((image = ImageCreate(IMAGE_GRAY16, WIDTH, HEIGHT)) == NULL ||
	    (buffer = malloc((sizeof(double)) * WIDTH * HEIGHT)) == NULL)
	{
		StatusSet(&st, NULL, STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	double value = 0.0;
	double min = 0.0;
	double max = 0.0;

	double scale = 1000.0 / (double)SCALE;
	double min_dimension = sMin((double)image->width, (double)image->height);

	// Generate heightmap
	printf("Generating heightmap...\n");

	for (size_t row = 0; row < image->height; row++)
	{
		for (size_t col = 0; col < image->width; col++)
		{
			value = Simplex2d(&s_perm_a, (row / min_dimension) * scale, (col / min_dimension) * scale);
			value += Simplex2d(&s_perm_b, (row / min_dimension) * scale * 2.0, (col / min_dimension) * scale * 2.0) / 2.0;
			value += Simplex2d(&s_perm_c, (row / min_dimension) * scale * 4.0, (col / min_dimension) * scale * 4.0) / 4.0;
			value += Simplex2d(&s_perm_d, (row / min_dimension) * scale * 8.0, (col / min_dimension) * scale * 8.0) / 8.0;
			value += Simplex2d(&s_perm_e, (row / min_dimension) * scale * 16.0, (col / min_dimension) * scale * 16.0) / 16.0;
			value *= 1.0 / (1.0 + (1.0 / 2.0) + (1.0 / 4.0) + (1.0 / 8.0) + (1.0 / 16.0));

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
		buffer[i] = buffer[i] * (1.0 / max);

		if (buffer[i] < -1.0)
			buffer[i] = -1.0;

		if (buffer[i] > 1.0)
			buffer[i] = 1.0;
	}

	// Simulate hydraulic erosion
	{
		printf(" - Simulating hydraulic erosion...\n");

		struct ErodeOptions options = {0};
		options.n = 50000;
		options.ttl = 30;
		options.p_radius = 4;
		options.p_enertia = 0.1;
		options.p_capacity = 50;
		options.p_gravity = 4;
		options.p_evaporation = 0.1;
		options.p_erosion = 0.1;
		options.p_deposition = 1;
		options.p_min_slope = 0.0001;

		HydraulicErosion(WIDTH, HEIGHT, buffer, &options);
	}

	// Save heightmap
	{
		printf("Saving heightmap...\n");
		uint16_t* pixel = image->data;

		for (size_t i = 0; i < (WIDTH * HEIGHT); i++)
			pixel[i] = (uint16_t)((buffer[i] + 1.0) * 32767.5);

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
