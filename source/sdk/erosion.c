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

#include "erosion.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


struct Vector2l
{
	double x;
	double y;
};

static inline struct Vector2l Vector2lAdd(struct Vector2l a, struct Vector2l b)
{
	return (struct Vector2l){(a.x + b.x), (a.y + b.y)};
}

static inline struct Vector2l Vector2lSubtract(struct Vector2l a, struct Vector2l b)
{
	return (struct Vector2l){(a.x - b.x), (a.y - b.y)};
}

static inline struct Vector2l Vector2lScale(struct Vector2l v, double scale)
{
	return (struct Vector2l){(v.x * scale), (v.y * scale)};
}

static inline struct Vector2l Vector2lNormalize(struct Vector2l v)
{
	double length = sqrt(pow(v.x, 2.0f) + pow(v.y, 2.0f));
	return (struct Vector2l){(v.x / length), (v.y / length)};
}

struct Particle
{
	struct Vector2l pos;
	struct Vector2l dir;
	double vel;
	double sediment;
	double water;
};


struct HgTuple
{
	struct Vector2l gradient;
	double height;
};


static inline double sMax(double a, double b)
{
	return (a > b) ? a : b;
}


static inline double sMin(double a, double b)
{
	return (a < b) ? a : b;
}


double BilinearInterpolation(int width, int height, const double* hmap, struct Vector2l pos)
{
	if (pos.x != pos.x || pos.y != pos.y)
		return 0.0;

	(void)height;
	double u, v, ul, ur, ll, lr, ipl_l, ipl_r;

	int x_i = (int)(pos.x);
	int y_i = (int)(pos.y);

	u = pos.x - x_i;
	v = pos.y - y_i;

	ul = hmap[y_i * width + x_i];
	ur = hmap[y_i * width + x_i + 1];

	ll = hmap[(y_i + 1) * width + x_i];
	lr = hmap[(y_i + 1) * width + x_i + 1];

	ipl_l = (1 - v) * ul + v * ll;
	ipl_r = (1 - v) * ur + v * lr;

	return (1 - u) * ipl_l + u * ipl_r;
}


static void sDeposit(int width, int height, double* hmap, struct Vector2l pos, double amount)
{
	(void)height;

	// - Deposits sediment at position `pos` in heighmap `hmap`.
	// - Deposition only affect immediate neighbouring gridpoints to `pos`.

	int x_i = (int)pos.x;
	int y_i = (int)pos.y;
	double u = pos.x - x_i;
	double v = pos.y - y_i;

	hmap[y_i * width + x_i] += amount * (1 - u) * (1 - v);
	hmap[y_i * width + x_i + 1] += amount * u * (1 - v);
	hmap[(y_i + 1) * width + x_i] += amount * (1 - u) * v;
	hmap[(y_i + 1) * width + x_i + 1] += amount * u * v;
}


static void sErode(int width, int height, double* hmap, struct Vector2l pos, double amount, int radius)
{
	// - Erodes heighmap `hmap` at position `pos` by amount `amount`
	// - Erosion is distributed over an area defined through p_radius

	if (radius < 1)
	{
		sDeposit(width, height, hmap, pos, -amount);
		return;
	}

	int x0 = (int)pos.x - radius;
	int y0 = (int)pos.y - radius;
	int x_start = sMax(0, x0);
	int y_start = sMax(0, y0);
	int x_end = sMin(width, x0 + 2 * radius + 1);
	int y_end = sMin(height, y0 + 2 * radius + 1);

	// Construct erosion/deposition kernel
	double kernel[2 * radius + 1][2 * radius + 1];
	double kernel_sum = 0;

	for (int y = y_start; y < y_end; y++)
	{
		for (int x = x_start; x < x_end; x++)
		{
			double d_x = x - pos.x;
			double d_y = y - pos.y;
			double distance = sqrt(d_x * d_x + d_y * d_y);
			double w = fmax(0, radius - distance);
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


static struct Vector2l sGradientAt(int width, int height, const double* hmap, int x, int y)
{
	int idx = y * width + x;
	// int right = y * hmap->width + min(x, hmap->width - 2);
	// int below = min(y, hmap->height - 2) * hmap->width + x;
	int right = idx + (((int)x > width - 2) ? 0 : 1);
	int below = idx + (((int)y > height - 2) ? 0 : (int)width);

	struct Vector2l g;
	g.x = hmap[right] - hmap[idx];
	g.y = hmap[below] - hmap[idx];

	return g;
}


static struct HgTuple sHeightGradientAt(int width, int height, const double* hmap, struct Vector2l pos)
{
	struct HgTuple ret;
	struct Vector2l ul, ur, ll, lr, ipl_l, ipl_r;
	int x_i = lrint(pos.x);
	int y_i = lrint(pos.y);
	double u = pos.x - x_i;
	double v = pos.y - y_i;

	ul = sGradientAt(width, height, hmap, x_i, y_i);
	ur = sGradientAt(width, height, hmap, x_i + 1, y_i);
	ll = sGradientAt(width, height, hmap, x_i, y_i + 1);
	lr = sGradientAt(width, height, hmap, x_i + 1, y_i + 1);
	ipl_l = Vector2lAdd(Vector2lScale(ul, 1 - v), Vector2lScale(ll, v));
	ipl_r = Vector2lAdd(Vector2lScale(ur, 1 - v), Vector2lScale(lr, v));
	ret.gradient = Vector2lAdd(Vector2lScale(ipl_l, 1 - u), Vector2lScale(ipl_r, u));
	ret.height = BilinearInterpolation(width, height, hmap, pos);

	return ret;
}


/*-----------------------------

 HydraulicErosion()
-----------------------------*/
void HydraulicErosion(int width, int height, double* hmap, struct ErodeOptions* options)
{
	// Simulate each struct Particle
	for (int i = 0; i < options->n; i++)
	{
		if (!((i + 1) % 10000))
			printf(" - Particles simulated: %d...\n", i + 1);

		// Spawn struct Particle
		struct Particle p;
		double denom = (RAND_MAX / ((double)width - 1.0));

		p.pos = (struct Vector2l){(double)rand() / denom, (double)rand() / denom};
		p.dir = (struct Vector2l){0, 0};
		p.vel = 0;
		p.sediment = 0;
		p.water = 1;

		for (int j = 0; j < options->ttl; j++)
		{
			// Interpolate gradient g and height h_old at p's position
			struct Vector2l pos_old = p.pos;
			struct HgTuple hg = sHeightGradientAt(width, height, hmap, pos_old);
			struct Vector2l g = hg.gradient;
			double h_old = hg.height;

			// Calculate new dir vector
			p.dir =
			    Vector2lSubtract(Vector2lScale(p.dir, options->p_enertia), Vector2lScale(g, 1 - options->p_enertia));
			p.dir = Vector2lNormalize(p.dir);

			// Calculate new pos
			p.pos = Vector2lAdd(p.pos, p.dir);

			// Check bounds
			struct Vector2l pos_new = p.pos;
			if (pos_new.x > (width - 1) || pos_new.x < 0 || pos_new.y > (height - 1) || pos_new.y < 0)
				break;

			// New height
			double h_new = BilinearInterpolation(width, height, hmap, pos_new);
			double h_diff = h_new - h_old;

			// Sediment capacity
			double c = fmax(-h_diff, options->p_min_slope) * p.vel * p.water * options->p_capacity;

			// Decide whether to sErode or deposit depending on struct Particle properties
			if (h_diff > 0 || p.sediment > c)
			{
				double to_deposit = (h_diff > 0) ? fmin(p.sediment, h_diff) : (p.sediment - c) * options->p_deposition;
				p.sediment -= to_deposit;
				sDeposit(width, height, hmap, pos_old, to_deposit);
			}
			else
			{
				double to_erode = fmin((c - p.sediment) * options->p_erosion, -h_diff);
				p.sediment += to_erode;
				sErode(width, height, hmap, pos_old, to_erode, options->p_radius);
			}

			// Update `vel` and `water`
			p.vel = sqrt(p.vel * p.vel + h_diff * options->p_gravity);
			p.water *= (1 - options->p_evaporation);
		}
	}
}
