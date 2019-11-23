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
		double p_enertia;
		double p_capacity;
		double p_gravity;
		double p_evaporation;
		double p_erosion;
		double p_deposition;
		double p_min_slope;
	};

	void HydraulicErosion(int width, int height, double* hmap, struct ErodeOptions* options);

#endif
