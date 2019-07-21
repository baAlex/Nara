/*-----------------------------

 [game.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef GAME_H
#define GAME_H

	#include <stdio.h>
	#include "entity.h"
	#include "context.h"
	#include "math.h"

	#ifndef M_PI
	#define M_PI 3.14159265358979323846264338327950288
	#endif

	#define DEG_TO_RAD(d) ((d)*M_PI / 180.0)
	#define RAD_TO_DEG(r) ((r)*180.0 / M_PI)

	// TODO: Too many globals!
	extern struct WindowSpecifications g_window_specs;
	extern struct TimeSpecifications g_time_specs;
	extern struct InputSpecifications g_input_specs;
	extern struct Context* g_context;


	struct GameCamera
	{
		struct Vector3 target;
		float distance;
	};

	void GameCameraStart(struct Entity* self);
	void GameCameraDelete(struct Entity* self);
	void GameCameraThink(struct Entity* self, float delta);


	struct GamePlayer
	{
		// At the moment working as placeholder
		float acceleration;
	};

	void GamePlayerStart(struct Entity* self);
	void GamePlayerDelete(struct Entity* self);
	void GamePlayerThink(struct Entity* self, float delta);

#endif
