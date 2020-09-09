/*-----------------------------

MIT License

Copyright (c) 2020 Alexander Brandt

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

 [nara.c]
 - Alexander Brandt 2020
-----------------------------*/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "SDL2/SDL.h"
#include "japan-image.h"
#include "japan-matrix.h"
#include "japan-version.h"
#include "kansai-context.h"
#include "kansai-version.h"

#define NAME "Nara"
#define VERSION "0.4-alpha"
#define CAPTION "Nara | v0.4-alpha"


struct NaraData
{
	int a;
};


static void sInit(struct kaWindow* w, void* raw_data, struct jaStatus* st)
{
	(void)w;
	(void)raw_data;
	(void)st;
}


static void sFrame(struct kaWindow* w, struct kaEvents e, float delta, void* raw_data, struct jaStatus* st)
{
	(void)w;
	(void)e;
	(void)delta;
	(void)raw_data;
	(void)st;
}


static void sResize(struct kaWindow* w, int width, int height, void* raw_data, struct jaStatus* st)
{
	(void)w;
	(void)width;
	(void)height;
	(void)raw_data;
	(void)st;
}


static void sFunctionKey(struct kaWindow* w, int f, void* raw_data, struct jaStatus* st)
{
	(void)w;
	(void)raw_data;
	(void)st;

	if (f == 11)
		kaSwitchFullscreen(w);
}


static void sClose(struct kaWindow* w, void* raw_data)
{
	(void)w;
	(void)raw_data;
}


int main(int argc, char* argv[])
{
	struct jaStatus st = {0};
	struct NaraData* data = NULL;
	SDL_version sdl_ver;

	// Game as normal
	SDL_GetVersion(&sdl_ver);

	printf("%s v%s\n", NAME, VERSION);
	printf(" - LibJapan %i.%i.%i\n", jaVersionMajor(), jaVersionMinor(), jaVersionPatch());
	printf(" - LibKansai %i.%i.%i\n", kaVersionMajor(), kaVersionMinor(), kaVersionPatch());
	printf(" - SDL2 %i.%i.%i\n", sdl_ver.major, sdl_ver.minor, sdl_ver.patch);

	if ((data = calloc(1, sizeof(struct NaraData))) == NULL)
		goto return_failure;

	if (kaContextStart(&st) != 0)
		goto return_failure;

	if (kaWindowCreate(CAPTION, sInit, sFrame, sResize, sFunctionKey, sClose, data, &st) != 0)
		goto return_failure;

	// Main loop
	while (1)
	{
		if (kaContextUpdate(&st) != 0)
			break;
	}

	if (st.code != JA_STATUS_SUCCESS)
		goto return_failure;

	// Bye!
	kaContextStop();
	return EXIT_SUCCESS;

return_failure:
	jaStatusPrint(NAME, st);
	kaContextStop();
	return EXIT_FAILURE;
}
