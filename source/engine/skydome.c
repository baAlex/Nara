/*-----------------------------

MIT License

Copyright (c) 2020 Alexander Brandt

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

 [skydome.c]
 - Alexander Brandt 2020

 https://en.wikipedia.org/wiki/Regular_icosahedron
 http://www.songho.ca/opengl/gl_sphere.html ( the graphics! <3 )
-----------------------------*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "japan-utilities.h"
#include "kansai-color.h"
#include "skydome.h"

#define BASE_VERTICES 12
#define BASE_INDICES 60


struct Skydome* SkydomeCreate(struct kaWindow* w, float radius, int iterations, struct jaBuffer* temp_buffer,
                              struct jaStatus* st)
{
	struct Skydome* skydome = NULL;
	struct kaVertex* vertex = NULL;
	uint16_t* index = NULL;

	jaStatusSet(st, "SkydomeCreate", JA_STATUS_SUCCESS, NULL);

	if ((skydome = calloc(1, sizeof(struct Skydome))) == NULL)
	{
		jaStatusSet(st, "SkydomeCreate", JA_STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	// Create an icosphere/icosahedron
	{
		struct KA_RANDOM_STATE random_state = {123};
		size_t size = BASE_VERTICES * sizeof(struct kaVertex) + BASE_INDICES * sizeof(uint16_t);

		if (jaBufferResize(temp_buffer, size) == NULL)
		{
			jaStatusSet(st, "SkydomeCreate", JA_STATUS_MEMORY_ERROR, NULL);
			goto return_failure;
		}

		vertex = temp_buffer->data;
		index = (uint16_t*)((struct kaVertex*)temp_buffer->data + BASE_VERTICES);

		// Upper cone' (pentagonal pyramid), 5 base vertices spaced at 72º
		for (int i = 0; i < 5; i++)
		{
			vertex[i].position.x = radius * cosf(atanf(1.0f / 2.0f)) * cosf(jaDegToRad(72.0f * (float)i));
			vertex[i].position.y = radius * cosf(atanf(1.0f / 2.0f)) * sinf(jaDegToRad(72.0f * (float)i));
			vertex[i].position.z = radius * sinf(atanf(1.0f / 2.0f));

			vertex[i].color = kaRgbaRandom(&random_state, 1.0f);

			index[i * 3 + 0] = (i != 4) ? (i + 1) : 0; // 0 = First vertex on the base
			index[i * 3 + 1] = i;
			index[i * 3 + 2] = 10; // 10 = Cone tip vertex
		}

		// Bottom cone, same except that the base spacing has an offset of 36º
		for (int i = 5; i < 10; i++)
		{
			vertex[i].position.x = radius * cosf(atanf(1.0f / 2.0f)) * cosf(jaDegToRad(36.0f + 72.0f * (float)i));
			vertex[i].position.y = radius * cosf(atanf(1.0f / 2.0f)) * sinf(jaDegToRad(36.0f + 72.0f * (float)i));
			vertex[i].position.z = radius * sinf(-atanf(1.0f / 2.0f));

			vertex[i].color = kaRgbaRandom(&random_state, 1.0f);

			index[i * 3 + 0] = 11; // 11 = Cone tip vertex
			index[i * 3 + 1] = i;
			index[i * 3 + 2] = (i != 9) ? (i + 1) : 5; // 5 = First vertex on the base
		}

		// Cones, upper and bottom, tip vertices
		vertex[10].position = (struct jaVector3){0.0f, 0.0f, radius};
		vertex[11].position = (struct jaVector3){0.0f, 0.0f, -radius};
		vertex[10].color = kaRgbaRandom(&random_state, 1.0f);
		vertex[11].color = kaRgbaRandom(&random_state, 1.0f);

		// Side faces joining cones bases
		for (int i = 0; i < 5; i++)
		{
			// Top to bottom
			index[30 + i * 6 + 0] = (i != 4) ? (i + 1) : 0;
			index[30 + i * 6 + 1] = i + 5;
			index[30 + i * 6 + 2] = i;

			// Bottom to top
			index[30 + i * 6 + 3] = i + 5;
			index[30 + i * 6 + 4] = (i != 4) ? (i + 1) : 0;
			index[30 + i * 6 + 5] = (i != 4) ? (i + 5 + 1) : 5;
		}
	}

	// Subdivide it
	{
		// TODO :)
	}

	// Finally the OpenGL object
	if (kaIndexInit(w, index, BASE_INDICES, &skydome->index, st) != 0 ||
	    kaVerticesInit(w, vertex, BASE_VERTICES, &skydome->vertices, st) != 0)
		goto return_failure;

	// Bye!
	return skydome;
return_failure:
	return NULL;
}


void SkydomeDelete(struct kaWindow* w, struct Skydome* skydome)
{
	kaIndexFree(w, &skydome->index);
	kaVerticesFree(w, &skydome->vertices);
	free(skydome);
}
