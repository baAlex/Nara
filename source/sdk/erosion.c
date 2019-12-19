/*-----------------------------

MIT License

Copyright (c) 2019 Henrik A. Glass

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

 [erosion.c]
 - Henrik A. Glass 2019

 Based on: https://github.com/henrikglass/erodr
-----------------------------*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "japan-utilities.h"
#include "japan-vector.h"

#include "erosion.h"


struct Particle
{
	struct jaVector2 pos;
	struct jaVector2 dir;
	float vel;
	float sediment;
	float water;
};

struct HgTuple
{
	struct jaVector2 gradient;
	float height;
};


static float sBilinearInterpolation(int width, int height, const float* hmap, struct jaVector2 pos)
{
	if (pos.x != pos.x || pos.y != pos.y)
		return 0.0f;

	(void)height;
	float u, v, ul, ur, ll, lr, ipl_l, ipl_r;

	int x_i = (int)floorf(pos.x);
	int y_i = (int)floorf(pos.y);

	u = pos.x - floorf(pos.x);
	v = pos.y - floorf(pos.y);

	ul = hmap[y_i * width + x_i];
	ur = hmap[y_i * width + x_i + 1];

	ll = hmap[(y_i + 1) * width + x_i];
	lr = hmap[(y_i + 1) * width + x_i + 1];

	ipl_l = (1 - v) * ul + v * ll;
	ipl_r = (1 - v) * ur + v * lr;

	return (1 - u) * ipl_l + u * ipl_r;
}


static void sDeposit(int width, int height, float* hmap, struct jaVector2 pos, float amount)
{
	(void)height;

	// - Deposits sediment at position `pos` in heighmap `hmap`.
	// - Deposition only affect immediate neighbouring gridpoints to `pos`.

	int x_i = (int)floorf(pos.x);
	int y_i = (int)floorf(pos.y);

	float u = pos.x - floorf(pos.x);
	float v = pos.y - floorf(pos.y);

	hmap[y_i * width + x_i] += amount * (1.0f - u) * (1.0f - v);
	hmap[y_i * width + x_i + 1] += amount * u * (1.0f - v);
	hmap[(y_i + 1) * width + x_i] += amount * (1.0f - u) * v;
	hmap[(y_i + 1) * width + x_i + 1] += amount * u * v;
}


static void sErode(int width, int height, float* hmap, struct jaVector2 pos, float amount, int radius)
{
	// - Erodes heighmap `hmap` at position `pos` by amount `amount`
	// - Erosion is distributed over an area defined through p_radius

	if (radius < 1)
	{
		sDeposit(width, height, hmap, pos, -amount);
		return;
	}

	int x0 = (int)floorf(pos.x) - radius;
	int y0 = (int)floorf(pos.y) - radius;
	int x_start = jaMax(0, x0);
	int y_start = jaMax(0, y0);
	int x_end = jaMin(width, x0 + 2 * radius + 1);
	int y_end = jaMin(height, y0 + 2 * radius + 1);

	// Construct erosion/deposition kernel
	float kernel[2 * radius + 1][2 * radius + 1];
	float kernel_sum = 0.0f;

	for (int y = y_start; y < y_end; y++)
	{
		for (int x = x_start; x < x_end; x++)
		{
			float d_x = (float)x - pos.x;
			float d_y = (float)y - pos.y;
			float distance = sqrtf(d_x * d_x + d_y * d_y);
			float w = fmaxf(0.0f, (float)radius - distance);
			kernel_sum += w;
			kernel[y - y0][x - x0] = w;
		}
	}

	// Normalize weights and apply changes on heighmap
	for (int y = y_start; y < y_end; y++)
	{
		for (int x = x_start; x < x_end; x++)
		{
			kernel[y - y0][x - x0] /= kernel_sum;
			hmap[y * width + x] -= amount * kernel[y - y0][x - x0];
		}
	}
}


static struct jaVector2 sGradientAt(int width, int height, const float* hmap, int x, int y)
{
	x = x % width;
	y = y % height;

	int idx = y * width + x;
	// int right = y * hmap->width + min(x, hmap->width - 2);
	// int below = min(y, hmap->height - 2) * hmap->width + x;
	int right = idx + ((x > width - 2) ? 0 : 1);
	int below = idx + ((y > height - 2) ? 0 : width);

	struct jaVector2 g;
	g.x = hmap[right] - hmap[idx];
	g.y = hmap[below] - hmap[idx];

	return g;
}


static struct HgTuple sHeightGradientAt(int width, int height, const float* hmap, struct jaVector2 pos)
{
	struct HgTuple ret;
	struct jaVector2 ul, ur, ll, lr, ipl_l, ipl_r;
	int x_i = (int)lround(pos.x);
	int y_i = (int)lround(pos.y);
	float u = pos.x - floorf(pos.x);
	float v = pos.y - floorf(pos.y);

	ul = sGradientAt(width, height, hmap, x_i, y_i);
	ur = sGradientAt(width, height, hmap, x_i + 1, y_i);
	ll = sGradientAt(width, height, hmap, x_i, y_i + 1);
	lr = sGradientAt(width, height, hmap, x_i + 1, y_i + 1);
	ipl_l = jaVector2Add(jaVector2Scale(ul, 1.0f - v), jaVector2Scale(ll, v));
	ipl_r = jaVector2Add(jaVector2Scale(ur, 1.0f - v), jaVector2Scale(lr, v));
	ret.gradient = jaVector2Add(jaVector2Scale(ipl_l, 1.0f - u), jaVector2Scale(ipl_r, u));
	ret.height = sBilinearInterpolation(width, height, hmap, pos);

	return ret;
}


/*-----------------------------

 HydraulicErosion()
-----------------------------*/
void HydraulicErosion(int width, int height, float* hmap, struct ErodeOptions* options)
{
	// Simulate each struct Particle
	for (int i = 0; i < options->n; i++)
	{
		if (!((i + 1) % 10000))
			printf(" - Particles simulated: %d...\n", i + 1);

		// Spawn struct Particle
		struct Particle p;
		float denom = ((float)RAND_MAX / ((float)width - 1.0f));

		p.pos = (struct jaVector2){(float)rand() / denom, (float)rand() / denom};
		p.dir = (struct jaVector2){0.0f, 0.0f};
		p.vel = 0.0f;
		p.sediment = 0.0f;
		p.water = 1.0f;

		for (int j = 0; j < options->ttl; j++)
		{
			// Interpolate gradient g and height h_old at p's position
			struct jaVector2 pos_old = p.pos;
			struct HgTuple hg = sHeightGradientAt(width, height, hmap, pos_old);
			struct jaVector2 g = hg.gradient;
			float h_old = hg.height;

			// Calculate new dir vector
			p.dir =
			    jaVector2Subtract(jaVector2Scale(p.dir, options->p_enertia), jaVector2Scale(g, 1 - options->p_enertia));
			p.dir = jaVector2Normalize(p.dir);

			// Calculate new pos
			p.pos = jaVector2Add(p.pos, p.dir);

			// Check bounds
			struct jaVector2 pos_new = p.pos;
			if (pos_new.x > (float)(width - 1) || pos_new.x < 0.0f || pos_new.y > (float)(height - 1) ||
			    pos_new.y < 0.0f)
				break;

			// New height
			float h_new = sBilinearInterpolation(width, height, hmap, pos_new);
			float h_diff = h_new - h_old;

			// Sediment capacity
			float c = fmaxf(-h_diff, options->p_min_slope) * p.vel * p.water * options->p_capacity;

			// Decide whether to sErode or deposit depending on struct Particle properties
			if (h_diff > 0.0f || p.sediment > c)
			{
				float to_deposit =
				    (h_diff > 0.0f) ? fminf(p.sediment, h_diff) : (p.sediment - c) * options->p_deposition;
				p.sediment -= to_deposit;
				sDeposit(width, height, hmap, pos_old, to_deposit);
			}
			else
			{
				float to_erode = fminf((c - p.sediment) * options->p_erosion, -h_diff);
				p.sediment += to_erode;
				sErode(width, height, hmap, pos_old, to_erode, options->p_radius);
			}

			// Update `vel` and `water`
			p.vel = sqrtf(p.vel * p.vel + h_diff * options->p_gravity);
			p.water *= (1.0f - options->p_evaporation);
		}
	}
}
