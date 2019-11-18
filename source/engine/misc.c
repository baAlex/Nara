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

 [misc.c]
 - Alexander Brandt 2019
-----------------------------*/

#include "misc.h"
#include "math.h"

#include <stddef.h> // FIXME, "utilities.h" in libJapan requires it!
#include "utilities.h"


void VectorAxes(struct Vector3 angle, struct Vector3* forward, struct Vector3* left, struct Vector3* up)
{
	// http://www.songho.ca/opengl/gl_anglestoaxes.html#anglestoaxes
	// Saving all diferences on axis disposition

	// x = Pith
	// y = Roll
	// z = Yaw

	float cx = cosf(DegToRad(angle.x));
	float sx = sinf(DegToRad(angle.x));
	float cy = cosf(DegToRad(angle.y)); // TODO: broken... maybe
	float sy = sinf(DegToRad(angle.y)); // "
	float cz = cosf(DegToRad(angle.z));
	float sz = sinf(DegToRad(angle.z));

	forward->x = sz * -sx;
	forward->y = cz * -sx;
	forward->z = -cx;

	left->x = (sz * sy * sx) + (cy * cz);
	left->y = (cz * sy * sx) + (cy * -sz);
	left->z = sx * sy;

	up->x = (sz * cy * cx) + -(sy * cz);
	up->y = (cz * cy * cx) + -(sy * -sz);
	up->z = -sx * cy;
}
