/*-----------------------------

 [entity.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef ENTITY_H
#define ENTITY_H

	#include <stddef.h>
	#include <stdbool.h>

	#include "vector.h"
	#include "list.h"
	#include "dictionary.h"

	struct Entity;

	struct EntityInput
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

	struct EntityCommon
	{
		struct Vector3 position;
		struct Vector3 angle;
	};

	struct Class
	{
		struct DictionaryItem* item;

		void* (*func_start)(const struct EntityCommon*);
		void (*func_delete)(void*);
		int (*func_think)(void*, const struct EntityInput*, struct EntityCommon*);

		size_t references;
		bool to_delete;
	};

	struct Entity
	{
		struct ListItem* item;
		struct Class* class;

		void* blob;

		struct EntityCommon co;
		struct EntityCommon old_co;

		bool to_start;
		bool to_delete;
	};

	struct Class* ClassCreate(struct Dictionary*, const char* name);
	struct Class* ClassGet(struct Dictionary*, const char* name);
	void ClassDelete(struct Class*);

	struct Entity* EntityCreate(struct List*, struct Class* class);
	void EntityDelete(struct Entity*); // Marks entity to be deleted after the update cycle

	void EntitiesUpdate(struct List*, struct EntityInput input); // TODO: pass input as pointer

#endif
