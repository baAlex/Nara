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

 Based on: https://gist.github.com/KdotJPG/b1270127455a94ac5d19

-----------------------------*/

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "image.h"


#define STRETCH_CONSTANT_2D -0.211324865405187
#define SQUISH_CONSTANT_2D 0.366025403784439
#define NORM_CONSTANT_2D 47.0


static int16_t s_perm[256];

static int8_t s_grandients[] = {
    5, 2, 2, 5, -5, 2, -2, 5, 5, -2, 2, -5, -5, -2, -2, -5,
};


static inline double sExtrapolate(int xsb, int ysb, double dx, double dy)
{
	int index = s_perm[(s_perm[xsb & 0xFF] + ysb) & 0xFF] & 0x0E;
	return s_grandients[index] * dx + s_grandients[index + 1] * dy;
}


static inline double sMin(double a, double b)
{
	return (a < b) ? a : b;
}


/*-----------------------------

 OpenSimplexSeed()
-----------------------------*/
void OpenSimplexSeed(long seed)
{
	uint16_t source[256];

	seed = seed * 6364136223846793005l + 1442695040888963407l;
	seed = seed * 6364136223846793005l + 1442695040888963407l;
	seed = seed * 6364136223846793005l + 1442695040888963407l;

	for (int i = 0; i < 256; i++)
		source[i] = i;

	for (int i = 255; i >= 0; i--)
	{
		seed = seed * 6364136223846793005l + 1442695040888963407l;

		int r = (int)((seed + 31) % (i + 1));

		if (r < 0)
			r += (i + 1);

		s_perm[i] = source[r];
		source[r] = source[i];
	}
}


/*-----------------------------

 OpenSimplex2d()
-----------------------------*/
double OpenSimplex2d(double x, double y)
{
	// Place input coordinates onto grid.
	double stretchOffset = (x + y) * STRETCH_CONSTANT_2D;
	double xs = x + stretchOffset;
	double ys = y + stretchOffset;

	// Floor to get grid coordinates of rhombus (stretched square) super-cell origin.
	int xsb = floor(xs);
	int ysb = floor(ys);

	// Skew out to get actual coordinates of rhombus origin. We'll need these later.
	double squishOffset = (xsb + ysb) * SQUISH_CONSTANT_2D;
	double xb = xsb + squishOffset;
	double yb = ysb + squishOffset;

	// Compute grid coordinates relative to rhombus origin.
	double xins = xs - xsb;
	double yins = ys - ysb;

	// Sum those together to get a value that determines which region we're in.
	double inSum = xins + yins;

	// Positions relative to origin point.
	double dx0 = x - xb;
	double dy0 = y - yb;

	// We'll be defining these inside the next block and using them afterwards.
	double dx_ext, dy_ext;
	int xsv_ext, ysv_ext;

	double value = 0;

	// Contribution (1,0)
	double dx1 = dx0 - 1.0 - SQUISH_CONSTANT_2D;
	double dy1 = dy0 - 0.0 - SQUISH_CONSTANT_2D;
	double attn1 = 2.0 - dx1 * dx1 - dy1 * dy1;
	if (attn1 > 0.0)
	{
		attn1 *= attn1;
		value += attn1 * attn1 * sExtrapolate(xsb + 1.0, ysb + 0.0, dx1, dy1);
	}

	// Contribution (0,1)
	double dx2 = dx0 - 0.0 - SQUISH_CONSTANT_2D;
	double dy2 = dy0 - 1.0 - SQUISH_CONSTANT_2D;
	double attn2 = 2.0 - dx2 * dx2 - dy2 * dy2;
	if (attn2 > 0.0)
	{
		attn2 *= attn2;
		value += attn2 * attn2 * sExtrapolate(xsb + 0, ysb + 1, dx2, dy2);
	}

	if (inSum <= 1.0)
	{
		// We're inside the triangle (2-Simplex) at (0,0)
		double zins = 1.0 - inSum;

		if (zins > xins || zins > yins)
		{
			//(0,0) is one of the closest two triangular vertices
			if (xins > yins)
			{
				xsv_ext = xsb + 1;
				ysv_ext = ysb - 1;
				dx_ext = dx0 - 1.0;
				dy_ext = dy0 + 1.0;
			}
			else
			{
				xsv_ext = xsb - 1;
				ysv_ext = ysb + 1;
				dx_ext = dx0 + 1.0;
				dy_ext = dy0 - 1.0;
			}
		}
		else
		{
			//(1,0) and (0,1) are the closest two vertices.
			xsv_ext = xsb + 1;
			ysv_ext = ysb + 1;
			dx_ext = dx0 - 1.0 - 2.0 * SQUISH_CONSTANT_2D;
			dy_ext = dy0 - 1.0 - 2.0 * SQUISH_CONSTANT_2D;
		}
	}
	else
	{
		// We're inside the triangle (2-Simplex) at (1,1)
		double zins = 2 - inSum;

		if (zins < xins || zins < yins)
		{
			//(0,0) is one of the closest two triangular vertices
			if (xins > yins)
			{
				xsv_ext = xsb + 2;
				ysv_ext = ysb + 0;
				dx_ext = dx0 - 2.0 - 2.0 * SQUISH_CONSTANT_2D;
				dy_ext = dy0 + 0.0 - 2.0 * SQUISH_CONSTANT_2D;
			}
			else
			{
				xsv_ext = xsb + 0;
				ysv_ext = ysb + 2;
				dx_ext = dx0 + 0.0 - 2.0 * SQUISH_CONSTANT_2D;
				dy_ext = dy0 - 2.0 - 2.0 * SQUISH_CONSTANT_2D;
			}
		}
		else
		{
			//(1,0) and (0,1) are the closest two vertices.
			dx_ext = dx0;
			dy_ext = dy0;
			xsv_ext = xsb;
			ysv_ext = ysb;
		}

		xsb += 1;
		ysb += 1;
		dx0 = dx0 - 1.0 - 2.0 * SQUISH_CONSTANT_2D;
		dy0 = dy0 - 1.0 - 2.0 * SQUISH_CONSTANT_2D;
	}

	// Contribution (0,0) or (1,1)
	double attn0 = 2.0 - dx0 * dx0 - dy0 * dy0;
	if (attn0 > 0)
	{
		attn0 *= attn0;
		value += attn0 * attn0 * sExtrapolate(xsb, ysb, dx0, dy0);
	}

	// Extra Vertex
	double attn_ext = 2.0 - dx_ext * dx_ext - dy_ext * dy_ext;
	if (attn_ext > 0)
	{
		attn_ext *= attn_ext;
		value += attn_ext * attn_ext * sExtrapolate(xsv_ext, ysv_ext, dx_ext, dy_ext);
	}

	return value / NORM_CONSTANT_2D;
}


/*-----------------------------

 main()
-----------------------------*/
#define SEED 123456789
#define WIDTH 1024
#define HEIGHT 1024
#define SCALE 100

int main()
{
	struct Image* image = NULL;
	struct Status st = {0};

	OpenSimplexSeed(SEED);

	if ((image = ImageCreate(IMAGE_GRAY16, WIDTH, HEIGHT)) == NULL)
		return EXIT_FAILURE;

	uint16_t* pixel = image->data;

	double value = 0.0;
	double min = 0.0;
	double max = 0.0;

	double scale = 1000.0 / (double)SCALE;
	double min_dimension = sMin((double)image->width, (double)image->height);

	for (size_t row = 0; row < image->height; row++)
	{
		for (size_t col = 0; col < image->width; col++)
		{
			value = OpenSimplex2d((row / min_dimension) * scale, (col / min_dimension) * scale);

			if (value > max)
				max = value;

			if (value < min)
				min = value;

			pixel[col + image->width * row] = (uint16_t)(((double)value + 1.0) * 32767.5);
		}
	}

	printf("Min: %f, Max: %f\n", min, max);

	// Bye!
	if (st.code != STATUS_SUCCESS)
		StatusPrint("Simplex sketch", st);

	st = ImageSaveSgi(image, "output.sgi");
	ImageSaveRaw(image, "output.data"); // GIMP did not support 16bit Sgi images

	if (st.code != STATUS_SUCCESS)
		StatusPrint("Simplex sketch", st);

	ImageDelete(image);

	return EXIT_SUCCESS;
}
