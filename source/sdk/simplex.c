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

 [simplex.c]
 - Alexander Brandt 2019

 Based on: https://gist.github.com/KdotJPG/b1270127455a94ac5d19
-----------------------------*/

#include "simplex.h"
#include <math.h>

#define STRETCH_CONSTANT_2D -0.211324865405187f
#define SQUISH_CONSTANT_2D 0.366025403784439f
#define NORM_CONSTANT_2D 47.0f


static int8_t s_grandients[] = {
    5, 2, 2, 5, -5, 2, -2, 5, 5, -2, 2, -5, -5, -2, -2, -5,
};


static inline float sExtrapolate(const struct Permutations* permutations, int xsb, int ysb, float dx, float dy)
{
	int index = permutations->p[(permutations->p[xsb & 0xFF] + ysb) & 0xFF] & 0x0E;
	return s_grandients[index] * dx + s_grandients[index + 1] * dy;
}


/*-----------------------------

 SimplexSeed()
-----------------------------*/
void SimplexSeed(struct Permutations* permutations, long seed)
{
	int16_t source[256];

	seed = seed * 6364136223846793005l + 1442695040888963407l;
	seed = seed * 6364136223846793005l + 1442695040888963407l;
	seed = seed * 6364136223846793005l + 1442695040888963407l;

	for (int i = 0; i < 256; i++)
		source[i] = (int16_t)i;

	for (int i = 255; i >= 0; i--)
	{
		seed = seed * 6364136223846793005l + 1442695040888963407l;

		int r = (int)((seed + 31) % (i + 1));

		if (r < 0)
			r += (i + 1);

		permutations->p[i] = source[r];
		source[r] = source[i];
	}
}


/*-----------------------------

 Simplex2d()
-----------------------------*/
float Simplex2d(const struct Permutations* permutations, float x, float y)
{
	// Place input coordinates onto grid.
	float stretchOffset = (x + y) * STRETCH_CONSTANT_2D;
	float xs = x + stretchOffset;
	float ys = y + stretchOffset;

	// Floor to get grid coordinates of rhombus (stretched square) super-cell origin.
	int xsb = (int)floor(xs);
	int ysb = (int)floor(ys);

	// Skew out to get actual coordinates of rhombus origin. We'll need these later.
	float squishOffset = (float)(xsb + ysb) * SQUISH_CONSTANT_2D;
	float xb = (float)xsb + squishOffset;
	float yb = (float)ysb + squishOffset;

	// Compute grid coordinates relative to rhombus origin.
	float xins = xs - (float)xsb;
	float yins = ys - (float)ysb;

	// Sum those together to get a value that determines which region we're in.
	float inSum = xins + yins;

	// Positions relative to origin point.
	float dx0 = x - xb;
	float dy0 = y - yb;

	// We'll be defining these inside the next block and using them afterwards.
	float dx_ext, dy_ext;
	int xsv_ext, ysv_ext;

	float value = 0;

	// Contribution (1,0)
	float dx1 = dx0 - 1.0f - SQUISH_CONSTANT_2D;
	float dy1 = dy0 - 0.0f - SQUISH_CONSTANT_2D;
	float attn1 = 2.0f - dx1 * dx1 - dy1 * dy1;

	if (attn1 > 0.0f)
	{
		attn1 *= attn1;
		value += attn1 * attn1 * sExtrapolate(permutations, xsb + 1, ysb + 0, dx1, dy1);
	}

	// Contribution (0,1)
	float dx2 = dx0 - 0.0f - SQUISH_CONSTANT_2D;
	float dy2 = dy0 - 1.0f - SQUISH_CONSTANT_2D;
	float attn2 = 2.0f - dx2 * dx2 - dy2 * dy2;

	if (attn2 > 0.0f)
	{
		attn2 *= attn2;
		value += attn2 * attn2 * sExtrapolate(permutations, xsb + 0, ysb + 1, dx2, dy2);
	}

	if (inSum <= 1.0f)
	{
		// We're inside the triangle (2-Simplex) at (0,0)
		float zins = 1.0f - inSum;

		if (zins > xins || zins > yins)
		{
			//(0,0) is one of the closest two triangular vertices
			if (xins > yins)
			{
				xsv_ext = xsb + 1;
				ysv_ext = ysb - 1;
				dx_ext = dx0 - 1.0f;
				dy_ext = dy0 + 1.0f;
			}
			else
			{
				xsv_ext = xsb - 1;
				ysv_ext = ysb + 1;
				dx_ext = dx0 + 1.0f;
				dy_ext = dy0 - 1.0f;
			}
		}
		else
		{
			//(1,0) and (0,1) are the closest two vertices.
			xsv_ext = xsb + 1;
			ysv_ext = ysb + 1;
			dx_ext = dx0 - 1.0f - 2.0f * SQUISH_CONSTANT_2D;
			dy_ext = dy0 - 1.0f - 2.0f * SQUISH_CONSTANT_2D;
		}
	}
	else
	{
		// We're inside the triangle (2-Simplex) at (1,1)
		float zins = 2 - inSum;

		if (zins < xins || zins < yins)
		{
			//(0,0) is one of the closest two triangular vertices
			if (xins > yins)
			{
				xsv_ext = xsb + 2;
				ysv_ext = ysb + 0;
				dx_ext = dx0 - 2.0f - 2.0f * SQUISH_CONSTANT_2D;
				dy_ext = dy0 + 0.0f - 2.0f * SQUISH_CONSTANT_2D;
			}
			else
			{
				xsv_ext = xsb + 0;
				ysv_ext = ysb + 2;
				dx_ext = dx0 + 0.0f - 2.0f * SQUISH_CONSTANT_2D;
				dy_ext = dy0 - 2.0f - 2.0f * SQUISH_CONSTANT_2D;
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
		dx0 = dx0 - 1.0f - 2.0f * SQUISH_CONSTANT_2D;
		dy0 = dy0 - 1.0f - 2.0f * SQUISH_CONSTANT_2D;
	}

	// Contribution (0,0) or (1,1)
	float attn0 = 2.0f - dx0 * dx0 - dy0 * dy0;

	if (attn0 > 0)
	{
		attn0 *= attn0;
		value += attn0 * attn0 * sExtrapolate(permutations, xsb, ysb, dx0, dy0);
	}

	// Extra Vertex
	float attn_ext = 2.0f - dx_ext * dx_ext - dy_ext * dy_ext;

	if (attn_ext > 0)
	{
		attn_ext *= attn_ext;
		value += attn_ext * attn_ext * sExtrapolate(permutations, xsv_ext, ysv_ext, dx_ext, dy_ext);
	}

	return value / NORM_CONSTANT_2D;
}