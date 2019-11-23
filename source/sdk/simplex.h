/*-----------------------------

 [simplex.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef SIMPLEX_H
#define SIMPLEX_H

	#include <stdint.h>

	struct Permutations
	{
		int16_t p[256];
	};

	void SimplexSeed(struct Permutations*, long seed);
	float Simplex2d(const struct Permutations* permutations, float x, float y);

#endif
