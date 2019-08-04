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
	#include "game/game.h"

	struct Entity;

	struct Class
	{
		struct DictionaryItem* item;

		void* (*func_start)();
		void (*func_delete)(void* blob);
		struct EntityCommon (*func_think)(void* blob, const struct EntityInput*);

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

	void EntitiesUpdate(struct List*, struct EntityInput input);

#endif
