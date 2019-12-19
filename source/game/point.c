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

 [point.c]
 - Alexander Brandt 2019
-----------------------------*/

#include "game.h"


struct Point
{
	struct jaVector3 position;
	struct jaVector3 angle;
};


/*-----------------------------

 GamePointStart()
-----------------------------*/
void* GamePointStart(const struct EntityCommon* initial_state)
{
	struct Point* point = malloc(sizeof(struct Point));

	if (point != NULL)
	{
		point->position = initial_state->position;
		point->angle = initial_state->angle;
	}

	return point;
}


/*-----------------------------

 GamePointDelete()
-----------------------------*/
void GamePointDelete(void* blob)
{
	struct Point* point = blob;
	free(point);
}


/*-----------------------------

 GamePointThink()
-----------------------------*/
int GamePointThink(void* blob, const struct EntityInput* input, struct EntityCommon* out_state)
{
	(void)input;
	struct Point* point = blob;

	out_state->position = point->position;
	out_state->angle = point->angle;

	return 0;
}
