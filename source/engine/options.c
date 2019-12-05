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
#include <stdlib.h>

struct Options
{
	int nothing;
};


/*-----------------------------

 OptionsCreate()
-----------------------------*/
struct Options* OptionsCreate()
{
	return malloc(1);
}


/*-----------------------------

 OptionsDelete()
-----------------------------*/
void OptionsDelete(struct Options* options) {}


/*-----------------------------

 OptionsReadArguments()
-----------------------------*/
int OptionsReadArguments(struct Options* options, int argc, const char* argv[], struct Status* st)
{
	return 1;
}


/*-----------------------------

 OptionsReadFile()
-----------------------------*/
int OptionsReadFile(struct Options* options, const char* filename, struct Status* st)
{
	return 1;
}


/*-----------------------------

 OptionsRegister()
-----------------------------*/
int OptionsRegisterInt(struct Options* options, const char* name, int default_value, struct Status* st)
{
	return 1;
}

int OptionsRegisterBool(struct Options* options, const char* name, bool default_value, struct Status* st)
{
	return 1;
}

int OptionsRegisterFloat(struct Options* options, const char* name, float default_value, struct Status* st)
{
	return 1;
}

int OptionsRegisterString(struct Options* options, const char* name, const char* default_value, struct Status* st)
{
	return 1;
}


/*-----------------------------

 OptionsRetrieve()
-----------------------------*/
int OptionsRetrieveInt(const struct Options* options, const char* name, int* dest, struct Status* st)
{
	return 1;
}

int OptionsRetrieveBool(const struct Options* options, const char* name, bool* dest, struct Status* st)
{
	return 1;
}

int OptionsRetrieveFloat(const struct Options* options, const char* name, float* dest, struct Status* st)
{
	return 1;
}

int OptionsRetrieveString(const struct Options* options, const char* name, char* dest, struct Status* st)
{
	return 1;
}
