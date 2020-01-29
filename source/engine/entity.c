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
 - Alexander Brandt 2019-2020
-----------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "entity.h"


static void sFreeEntityFromList(struct jaListItem* item)
{
	struct Entity* entity = item->data;
	struct Class* class = entity->class;

	if (entity->class->func_delete != NULL)
		entity->class->func_delete(entity->blob);

	class->references--;

	if (class->to_delete == true && class->references == 0)
		ClassDelete(class);
}


/*-----------------------------

 ClassCreate()
-----------------------------*/
struct Class* ClassCreate(struct jaDictionary* dictionary, const char* name)
{
	struct jaDictionaryItem* item = NULL;
	struct Class* class = NULL;

	if ((item = jaDictionaryAdd(dictionary, name, NULL, sizeof(struct Class))) != NULL)
	{
		class = item->data;

		memset(class, 0, sizeof(struct Class));
		class->item = item;
	}

	return class;
}


/*-----------------------------

 ClassGet()
-----------------------------*/
inline struct Class* ClassGet(struct jaDictionary* dictionary, const char* name)
{
	struct jaDictionaryItem* item = NULL;
	return ((item = jaDictionaryGet(dictionary, name)) != NULL) ? (struct Class*)item->data : NULL;
}


/*-----------------------------

 ClassDelete()
-----------------------------*/
inline void ClassDelete(struct Class* class)
{
	if (class->references != 0)
	{
		class->to_delete = true;
		jaDictionaryDetach(class->item);
	}
	else
	{
		printf("Deleted entity '%s' : %p\n", class->item->key, (void*)class);
		jaDictionaryRemove(class->item);
	}
}


/*-----------------------------

 EntityCreate()
-----------------------------*/
struct Entity* EntityCreate(struct jaList* list, struct Class* class)
{
	struct jaListItem* item = NULL;
	struct Entity* entity = NULL;

	if ((item = jaListAdd(list, NULL, sizeof(struct Entity))) == NULL)
		return NULL;

	item->callback_delete = sFreeEntityFromList;
	entity = item->data;

	memset(entity, 0, sizeof(struct Entity));
	entity->item = item;
	entity->class = class;
	entity->to_start = true;

	class->references++;

	printf("Created entity '%s : %p'\n", class->item->key, (void*)entity);
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
void EntitiesUpdate(struct jaList* list, struct EntityInput input)
{
	struct jaListItem* item = NULL;
	struct Entity* entity = NULL;

	struct Entity* last_special_case = NULL;

	struct jaListState s = {0};
	s.start = list->first;
	s.reverse = false;

	// 'func_think' callbacks
	while ((item = jaListIterate(&s)) != NULL)
	{
		entity = item->data;

		if (entity->to_delete == true || entity->to_start == true)
			last_special_case = entity;
		else if (entity->class->func_think != NULL)
		{
			entity->old_co = entity->co;
			entity->class->func_think(entity->blob, &input, &entity->co); // TODO: Check failure
		}
	}

	// There are no entity to be started/deleted, bye!
	if (last_special_case == NULL)
		return;

	// A new cycle but in reverse, for the special cases
	s.start = last_special_case->item;
	s.reverse = true;

	while ((item = jaListIterate(&s)) != NULL)
	{
		entity = item->data;

		if (entity->to_start == true)
		{
			if (entity->class->func_start != NULL)
				entity->blob = entity->class->func_start(&entity->co);

			entity->to_start = false;
		}

		if (entity->to_delete == true)
		{
			sFreeEntityFromList(item);

			entity->item->callback_delete = NULL;
			jaListRemove(entity->item);
		}
	}
}
