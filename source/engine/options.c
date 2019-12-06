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

 [options.c]
 - Alexander Brandt 2019
-----------------------------*/

#include "options.h"
#include "dictionary.h"

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


enum SetBy
{
	SET_DEFAULT = 0,
	SET_BY_ARGUMENTS,
	SET_BY_FILE
};

enum Type
{
	TYPE_INT = 0,
	TYPE_FLOAT,
	TYPE_BOOL,
	TYPE_STRING
};

union Value {
	int integer;
	float floating;
	bool boolean;
	const char* string;
};

struct Cvar
{
	enum Type type;
	enum SetBy set_by;
	union Value value;
};


static void sPrintCallback(struct DictionaryItem* item, void* data)
{
	(void)data;
	struct Cvar* cvar = (struct Cvar*)item->data;

	switch (cvar->type)
	{
	case TYPE_INT:
		printf(" - %s = %i [i%s]\n", item->key, cvar->value.integer, (cvar->set_by == SET_DEFAULT) ? "" : ", (*)");
		break;
	case TYPE_FLOAT:
		printf(" - %s = %f [f%s]\n", item->key, cvar->value.floating, (cvar->set_by == SET_DEFAULT) ? "" : ", (*)");
		break;
	case TYPE_BOOL:
		printf(" - %s = %s [b%s]\n", item->key, (cvar->value.boolean == true) ? "true" : "false",
		       (cvar->set_by == SET_DEFAULT) ? "" : ", (*)");
		break;
	case TYPE_STRING:
		printf(" - %s = \"%s\" [s%s]\n", item->key, cvar->value.string, (cvar->set_by == SET_DEFAULT) ? "" : ", (*)");
	}
}


/*-----------------------------

 OptionsCreate()
-----------------------------*/
inline struct Options* OptionsCreate()
{
	return (struct Options*)DictionaryCreate(NULL);
}


/*-----------------------------

 OptionsDelete()
-----------------------------*/
inline void OptionsDelete(struct Options* options)
{
	DictionaryDelete((struct Dictionary*)options);
}


/*-----------------------------

 OptionsReadArguments()
-----------------------------*/
static inline void sStoreString(const char* org, const char** dest)
{
	// TODO
	*dest = org;
}

static inline int sStoreBool(const char* org, bool* dest)
{
	bool value = false;
	bool value_set = false;

	for (const char* c = org; *c != '\0'; c++)
	{
		if (value_set == false)
		{
			if (isspace((int)*c) == 0) // If not
			{
				if (strncmp(c, "true", strlen("true")) == 0)
				{
					value = true;
					value_set = true;
					c += strlen("true") - 1;
				}
				else if (strncmp(c, "false", strlen("false")) == 0)
				{
					value = false;
					value_set = true;
					c += strlen("false") - 1;
				}
				else
					return 1;
			}
		}
		else if (isspace((int)*c) == 0) // If not
			return 1;
	}

	*dest = value;
	return 0;
}

static inline int sStoreFloat(const char* org, float* dest)
{
	char* end = NULL;
	float value = strtof(org, &end); // "Past the last character interpreted"

	if (*end != '\0')
		for (; *end != '\0'; end++)
		{
			if (isspace((int)*end) == 0) // If not
				return 1;
		}

	*dest = value;
	return 0;
}

static inline int sStoreInt(const char* org, int* dest)
{
	char* end = NULL;
	long value = strtol(org, &end, 0);

	if (*end != '\0')
		for (; *end != '\0'; end++)
		{
			if (isspace((int)*end) == 0)
			{
				// Try with float
				float temp = 0.0f;

				if (sStoreFloat(org, &temp) != 0)
					return 1;

				value = lroundf(temp);
				break;
			}
		}

	if (value > INT_MAX || value < INT_MIN)
		return 1;

	*dest = (int)value;
	return 0;
}

void OptionsReadArguments(struct Options* options, int argc, const char* argv[])
{
	struct DictionaryItem* item = NULL;
	struct Cvar* cvar = NULL;
	union Value old_value;

	for (int i = 1; i < argc; i++)
	{
		// Check key
		if (argv[i][0] != '-')
		{
			printf("[Warning] Unexpected token '%s'\n", (argv[i]));
			continue;
		}

		if ((item = DictionaryGet((struct Dictionary*)options, (argv[i] + 1))) == NULL)
		{
			printf("[Warning] Unknown option '%s'\n", (argv[i] + 1));
			continue;
		}

		// Check value
		cvar = item->data;
		memcpy(&old_value, &cvar->value, sizeof(union Value));

		if ((i + 1) == argc)
		{
			printf("[Warning] No value for option '%s'\n", (argv[i] + 1));

			i++; // Important!
			continue;
		}

		switch (cvar->type)
		{
		case TYPE_INT:
			if (sStoreInt(argv[i + 1], &cvar->value.integer) != 0)
				printf("[Warning] Token '%s' can't be cast into a integer value as '%s' requires\n", argv[i + 1],
				       (argv[i] + 1));
			break;
		case TYPE_FLOAT:
			if (sStoreFloat(argv[i + 1], &cvar->value.floating) != 0)
				printf("[Warning] Token '%s' can't be cast into a decimal value as '%s' requires\n", argv[i + 1],
				       (argv[i] + 1));
			break;
		case TYPE_BOOL:
			if (sStoreBool(argv[i + 1], &cvar->value.boolean) != 0)
				printf("[Warning] Token '%s' can't be cast into a boolean value as '%s' requires\n", argv[i + 1],
				       (argv[i] + 1));
			break;
		case TYPE_STRING: sStoreString(argv[i + 1], &cvar->value.string);
		}

		if (memcmp(&old_value, &cvar->value, sizeof(union Value)) != 0) // Something change?
			cvar->set_by = SET_BY_ARGUMENTS;

		i++; // Important!
	}

#ifdef DEBUG
	printf("(OptionsReadArguments)\n");
	DictionaryIterate((struct Dictionary*)options, sPrintCallback, NULL);
#endif
}


/*-----------------------------

 OptionsReadFile()
-----------------------------*/
int OptionsReadFile(struct Options* options, const char* filename, struct Status* st)
{
	(void)st;

#ifdef DEBUG
	printf("(OptionsReadFile, '%s')\n", filename);
	DictionaryIterate((struct Dictionary*)options, sPrintCallback, NULL);
#endif

	return 1;
}


/*-----------------------------

 OptionsRegister()
-----------------------------*/
static struct Cvar* sOptionsRegister(struct Options* options, const char* key, struct Status* st)
{
	struct DictionaryItem* item = NULL;

	StatusSet(st, "sOptionsRegister", STATUS_SUCCESS, NULL);

	for (const char* c = key; *c != '\0'; c++)
	{
		if (isalnum((int)*c) == 0 // If not
		    && *c != '_')
		{
			StatusSet(st, "sOptionsRegister", STATUS_INVALID_ARGUMENT, "Only alphanumeric options names allowed");
			return NULL;
		}
	}

	if ((item = DictionaryAdd((struct Dictionary*)options, key, NULL, sizeof(struct Cvar))) == NULL)
	{
		StatusSet(st, "sOptionsRegister", STATUS_MEMORY_ERROR, NULL);
		return NULL;
	}

	memset(item->data, 0, sizeof(struct Cvar));
	return item->data;
}

inline int OptionsRegisterInt(struct Options* options, const char* key, int default_value, struct Status* st)
{
	struct Cvar* cvar = sOptionsRegister(options, key, st);

	if (cvar != NULL)
	{
		cvar->type = TYPE_INT;
		cvar->value.integer = default_value;
		return 0;
	}

	return 1;
}

inline int OptionsRegisterBool(struct Options* options, const char* key, bool default_value, struct Status* st)
{
	struct Cvar* cvar = sOptionsRegister(options, key, st);

	if (cvar != NULL)
	{
		cvar->type = TYPE_BOOL;
		cvar->value.boolean = default_value;
		return 0;
	}

	return 1;
}

inline int OptionsRegisterFloat(struct Options* options, const char* key, float default_value, struct Status* st)
{
	struct Cvar* cvar = sOptionsRegister(options, key, st);

	if (cvar != NULL)
	{
		cvar->type = TYPE_FLOAT;
		cvar->value.floating = default_value;
		return 0;
	}

	return 1;
}

inline int OptionsRegisterString(struct Options* options, const char* key, const char* default_value, struct Status* st)
{
	struct Cvar* cvar = sOptionsRegister(options, key, st);

	if (cvar != NULL)
	{
		cvar->type = TYPE_STRING;
		cvar->value.string = default_value;
		return 0;
	}

	return 1;
}


/*-----------------------------

 OptionsRetrieve()
-----------------------------*/
inline int OptionsRetrieveInt(const struct Options* options, const char* key, int* dest, struct Status* st)
{
	(void)options;
	(void)key;
	(void)dest;
	(void)st;

	return 1;
}

inline int OptionsRetrieveBool(const struct Options* options, const char* key, bool* dest, struct Status* st)
{
	(void)options;
	(void)key;
	(void)dest;
	(void)st;

	return 1;
}

inline int OptionsRetrieveFloat(const struct Options* options, const char* key, float* dest, struct Status* st)
{
	(void)options;
	(void)key;
	(void)dest;
	(void)st;

	return 1;
}

inline int OptionsRetrieveString(const struct Options* options, const char* key, char* dest, struct Status* st)
{
	(void)options;
	(void)key;
	(void)dest;
	(void)st;

	return 1;
}
