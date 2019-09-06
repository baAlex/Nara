/*-----------------------------

 [game.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef GAME_H
#define GAME_H

	#include <stdio.h>
	#include <stdlib.h>
	#include <stdbool.h>
	#include <math.h>

	#include "vector.h"
	#include "matrix.h"

	#ifndef M_PI
	#define M_PI 3.14159265358979323846264338327950288f
	#endif

	#define DEG_TO_RAD(d) ((d)*M_PI / 180.0f)
	#define RAD_TO_DEG(r) ((r)*180.0f / M_PI)

	#ifndef ENTITY_H
		struct EntityInput // Copy from 'entity.h'
		{
			bool a, b, x, y;
			bool lb, rb;
			bool view, menu, guide;
			bool ls, rs;

			struct { float h, v; } pad;
			struct { float h, v, t; } left_analog;
			struct { float h, v, t; } right_analog;

			float delta;
		};

		struct EntityCommon // Copy from 'entity.h'
		{
			struct Vector3 position;
			struct Vector3 angle;
		};
	#endif

	void* GamePointStart(const struct EntityCommon* initial_state);
	void GamePointDelete(void* blob);
	int GamePointThink(void* blob, const struct EntityInput* input, struct EntityCommon* out_state);

	void* GameCameraStart(const struct EntityCommon* initial_state);
	void GameCameraDelete(void* blob);
	int GameCameraThink(void* blob, const struct EntityInput* input, struct EntityCommon* out_state);

#endif
