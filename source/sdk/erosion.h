/*-----------------------------

 [erosion.h]
 - Henrik A. Glass 2019
-----------------------------*/

#ifndef EROSION_H
#define EROSION_H

	#include <stddef.h>

	struct ErodeOptions
	{
		int n;
		int ttl;
		int p_radius;
		float p_enertia;
		float p_capacity;
		float p_gravity;
		float p_evaporation;
		float p_erosion;
		float p_deposition;
		float p_min_slope;
	};

	void HydraulicErosion(int width, int height, float* hmap, struct ErodeOptions* options);

#endif
