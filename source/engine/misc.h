/*-----------------------------

 [misc.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef MISC_H
#define MISC_H

	#ifndef M_PI
	#define M_PI 3.14159265358979323846264338327950288f
	#endif

	float DegToRad(float value);
	float RadToDeg(float value);

	int Max_i(int a, int b);
	float Max_f(float a, float b);

	#define Max(a, b) _Generic((a), \
			int: Max_i, \
			float: Max_f, \
			default: Max_i \
		)(a, b)

	int Min_i(int a, int b);
	float Min_f(float a, float b);

	#define Min(a, b) _Generic((a), \
			int: Min_i, \
			float: Min_f, \
			default: Min_i \
		)(a, b)

	int Clamp_i(int v, int min, int max);
	float Clamp_f(float v, float min, float max);

	#define Clamp(value, min, max) _Generic((value), \
			int: Clamp_i, \
			float: Clamp_f, \
			default: Clamp_i \
		)(value, min, max)

#endif
