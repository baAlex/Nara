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

 [entity.c]
 - Alexander Brandt 2019
-----------------------------*/

#include "entity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void sFreeEntityFromList(struct ListItem* item)
{
	struct Entity* entity = item->data;
	struct Class* class = entity->class;

	class->references--;

	if (class->class_data_size != 0 && class->references == 0)
	{
		if (class->func_class_data_free != NULL)
			class->func_class_data_free(class->data);

		free(class->data);
	}

	if (entity->class->func_delete != NULL)
		entity->class->func_delete(entity);

	printf("(EntityFree) '%s : %p' freeded\n", class->item->key, (void*)entity);
}


/*-----------------------------

 ClassCreate()
-----------------------------*/
struct Class* ClassCreate(struct Dictionary* dictionary, const char* name)
{
	struct DictionaryItem* item = NULL;
	struct Class* class = NULL;

	if ((item = DictionaryAdd(dictionary, name, NULL, sizeof(struct Class))) != NULL)
	{
		class = item->data;

		memset(class, 0, sizeof(struct Class));
		class->item = item;
	}

	printf("(ClassCreate) '%s' created (%p)\n", name, (void*)class);
	return class;
}


/*-----------------------------

 ClassGet()
-----------------------------*/
inline struct Class* ClassGet(struct Dictionary* dictionary, const char* name)
{
	struct DictionaryItem* item = NULL;
	return ((item = DictionaryGet(dictionary, name)) != NULL) ? (struct Class*)item->data : NULL;
}


/*-----------------------------

 ClassDelete()
-----------------------------*/
inline void ClassDelete(struct Class* class)
{
	if (class->references != 0)
	{
		class->to_delete = true;
		DictionaryDetach(class->item);
	}
	else
	{
		printf("(ClassDelete) '%s' deleted (%p)\n", class->item->key, (void*)class);
		DictionaryRemove(class->item);
	}
}


/*-----------------------------

 EntityCreate()
-----------------------------*/
struct Entity* EntityCreate(struct List* list, struct Class* class, struct Vector3 position)
{
	struct ListItem* item = NULL;
	struct Entity* entity = NULL;

	if ((item = ListAdd(list, NULL, sizeof(struct Entity) + class->entity_data_size)) == NULL)
		return NULL;

	item->callback_delete = sFreeEntityFromList; // I am not sure about this
	entity = item->data;

	// Entity set
	memset(entity, 0, sizeof(struct Entity));
	entity->item = item;
	entity->class = class;
	entity->position = position;
	entity->to_start = true;

	if (class->entity_data_size != 0)
		entity->data = (struct Entity*)entity + 1;

	// Class set
	if (class->class_data_size != 0 && class->references == 0)
	{
		class->data = calloc(1, sizeof(class->class_data_size));

		if (class->func_class_data_alloc != NULL)
			class->func_class_data_alloc(class->data);
	}

	class->references++;

	printf("(EntityCreate) '%s : %p' created, pos: [%02f, %02f, %02f]\n", class->item->key, (void*)entity, position.x,
	       position.y, position.z);
	return entity;
}


/*-----------------------------

 EntityDelete()
-----------------------------*/
inline void EntityDelete(struct Entity* entity)
{
	entity->to_delete = true;
}


/*-----------------------------

 EntitiesUpdate()
-----------------------------*/
void EntitiesUpdate(struct List* list, float delta)
{
	struct ListItem* item = NULL;
	struct Entity* entity = NULL;

	struct Entity* last_special_case = NULL;

	struct ListState s = {0};
	s.start = list->first;
	s.reverse = false;

	// 'func_think' callbacks
	while ((item = ListIterate(&s)) != NULL)
	{
		entity = item->data;

		if (entity->to_delete == true || entity->to_start == true)
			last_special_case = entity;
		else if (entity->class->func_think != NULL)
			entity->class->func_think(entity, delta);
	}

	// There are no entity to be started/deleted, bye!
	if (last_special_case == NULL)
		return;

	// A new cycle but in reverse, for the special cases
	s.start = last_special_case->item;
	s.reverse = true;

	while ((item = ListIterate(&s)) != NULL)
	{
		entity = item->data;

		if (entity->to_start == true)
		{
			if (entity->class->func_start != NULL)
				entity->class->func_start(entity);

			entity->to_start = false;
		}

		if (entity->to_delete == true)
		{
			sFreeEntityFromList(entity->item);

			entity->item->callback_delete = NULL;
			ListRemove(entity->item);
		}
	}
}
