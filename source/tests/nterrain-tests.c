/*-----------------------------

 [nterrain-tests.c]
 - Alexander Brandt 2019
-----------------------------*/

#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <cmocka.h>

#include "../engine/nterrain.h"
#include "../engine/tiny-gl.h" // Mocked


int VerticesInit(struct Vertices* out, const struct Vertex* data, size_t length, struct Status* st)
{
	(void)data;
	out->glptr = 0;
	out->length = length;
	st->code = STATUS_SUCCESS;
	return 0;
}

void VerticesFree(struct Vertices* vertices)
{
	(void)vertices;
}

int IndexInit(struct Index* out, const uint16_t* data, size_t length, struct Status* st)
{
	(void)data;
	out->glptr = 0;
	out->length = length;
	st->code = STATUS_SUCCESS;
	return 0;
}

void IndexFree(struct Index* index)
{
	(void)index;
}


/*-----------------------------

 main()
-----------------------------*/
int main()
{
	return 0;
}
