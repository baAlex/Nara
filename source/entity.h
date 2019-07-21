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

	struct Class
	{
		struct DictionaryItem* item;

		void (*func_start)(struct Entity* self);
		void (*func_delete)(struct Entity* self);
		void (*func_think)(struct Entity* self, float delta);

		int (*func_class_data_alloc)(void* data);
		void (*func_class_data_free)(void* data);

		size_t entity_data_size; // For every single entity *individually*
		size_t class_data_size;  // A single chunk shared betwen all entities
		                         // of the same class (npcs who uses the same
		                         // model, should save it here)
		void* data;

		// Private:
		size_t references;
		bool to_delete;
	};

	struct Entity
	{
		struct ListItem* item;

		struct Class* class;
		void* data;

		struct Vector3 position;

		// Private:
		bool to_start;
		bool to_delete;
	};

	struct Class* ClassCreate(struct Dictionary*, const char* name);
	struct Class* ClassGet(struct Dictionary*, const char* name);
	void ClassDelete(struct Class*);

	struct Entity* EntityCreate(struct List*, struct Class* class, struct Vector3 position);
	void EntityDelete(struct Entity*); // Marks entity to be deleted after the callbacks cycle

	void EntitiesUpdate(struct List*, float delta);

#endif
