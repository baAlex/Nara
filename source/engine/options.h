/*-----------------------------

 [options.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef OPTIONS_H
#define OPTIONS_H

	#include <stdbool.h>
	#include "status.h"

	struct Options;

	struct Options* OptionsCreate();
	void OptionsDelete(struct Options*);

	int OptionsReadArguments(struct Options*, int argc, const char* argv[], struct Status*);
	int OptionsReadFile(struct Options*, const char* filename, struct Status*);

	int OptionsRegisterInt(struct Options*, const char* name, int default_value, struct Status*);
	int OptionsRegisterBool(struct Options*, const char* name, bool default_value, struct Status*);
	int OptionsRegisterFloat(struct Options*, const char* name, float default_value, struct Status*);
	int OptionsRegisterString(struct Options*, const char* name, const char* default_value, struct Status*);

	int OptionsRetrieveInt(const struct Options*, const char* name, int* dest, struct Status*);
	int OptionsRetrieveBool(const struct Options*, const char* name, bool* dest, struct Status*);
	int OptionsRetrieveFloat(const struct Options*, const char* name, float* dest, struct Status*);
	int OptionsRetrieveString(const struct Options*, const char* name, char* dest, struct Status*);

	#define OptionsRegister(options, name, value, st) _Generic((value), \
		int: OptionsRegisterInt, \
		bool: OptionsRegisterBool, \
		float: OptionsRegisterFloat, \
		char*: OptionsRegisterString, \
		const char*: OptionsRegisterString, \
		default: OptionsRegisterInt \
	)(options, name, value, st)

	#define OptionsRetrieve(options, name, dest, st) _Generic((dest), \
		int*: OptionsRetrieveInt, \
		bool*: OptionsRetrieveBool, \
		float*: OptionsRetrieveFloat, \
		char*: OptionsRetrieveString, \
		default: OptionsRetrieveInt \
	)(options, name, dest, st)

#endif
