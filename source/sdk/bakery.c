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

 [bakery.c]
 - Alexander Brandt 2020
-----------------------------*/

// $ clang ../lib-japan/source/endianness.c ../lib-japan/source/status.c ../lib-japan/source/utilities.c
// ../lib-japan/source/vector.c ../lib-japan/source/image/image.c ../lib-japan/source/image/format-sgi.c ./bakery.c
// -I./include -lm -std=c11 -Wall -Wextra -Wconversion -O3 -flto -msse -msse2 -msse3 -mssse3 -msse4 -ffast-math -o
// bakery

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "japan-image.h"
#include "japan-utilities.h"
#include "japan-vector.h"


struct ProgressState
{
	struct timespec previous_time;
	struct timespec current_time;
	bool first_print;
	int i;
};


void ProgressInit(struct ProgressState* state)
{
	memset(state, 0, sizeof(struct ProgressState));
	timespec_get(&state->previous_time, TIME_UTC);
}


void ProgressPrint(struct ProgressState* state, const char* preffix, float progress)
{
	timespec_get(&state->current_time, TIME_UTC);

	if (state->current_time.tv_sec > state->previous_time.tv_sec)
	{
		if (state->first_print == false)
		{
			printf("\n");
			state->first_print = true;
		}

		printf("\033[A%s%.2f%%\n", preffix, progress);
		state->previous_time = state->current_time;
	}
}


void ProgressStop(struct ProgressState* state, const char* preffix)
{
	printf("%s\033[A%s100.0%%\n", (state->first_print == false) ? "\n" : "", preffix);
	ProgressInit(state);
}


float Mix(float a, float b, float v)
{
	return a * (1.0f - v) + b * v;
}


struct jaVector2 MixVector2(struct jaVector2 a, struct jaVector2 b, float v)
{
	struct jaVector2 ret;

	ret.x = Mix(a.x, b.x, v);
	ret.y = Mix(a.y, b.y, v);

	return ret;
}


struct jaVector3 MixVector3(struct jaVector3 a, struct jaVector3 b, float v)
{
	struct jaVector3 ret;

	ret.x = Mix(a.x, b.x, v);
	ret.y = Mix(a.y, b.y, v);
	ret.z = Mix(a.z, b.z, v);

	return ret;
}


float Smoothstep(float edge0, float edge1, float v)
{
	float t = jaClamp((v - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}


float LinearStep(float edge0, float edge1, float v)
{
	return jaClamp((v - edge0) / (edge1 - edge0), 0.0f, 1.0f);
}


float BrightnessContrast(float value, float brightness, float contrast)
{
	return (value - 0.5f) * contrast + 0.5f + brightness;
}


float ImageNearCR(const struct jaImage* image, size_t ch, size_t col, size_t row)
{
	float* data = image->data;

	col = jaClamp(col, 0, image->width - 1);
	row = jaClamp(row, 0, image->height - 1);

	return data[(row * image->width + col) * image->channels + ch];

	/*	union {
	        uint8_t* u8;
	        uint16_t* u16;
	        float* f32;
	        void* raw;
	    } component;

	    component.raw = image->data;

	    col = jaClamp(col, 0, image->width - 1);
	    row = jaClamp(row, 0, image->height - 1);

	    if (image->format == IMAGE_U8)
	        return (float)component.u8[(row * image->width + col) * image->channels + ch] / (float)UINT8_MAX;
	    else if (image->format == IMAGE_U16)
	        return (float)component.u16[(row * image->width + col) * image->channels + ch] / (float)UINT16_MAX;
	    else if (image->format == IMAGE_FLOAT)
	        return (float)component.f32[(row * image->width + col) * image->channels + ch];

	    return -1.0f;*/
}


float ImageNearUV(const struct jaImage* image, size_t ch, float u, float v)
{
	return ImageNearCR(image, ch, (size_t)(u * (float)image->width), (size_t)(v * (float)image->height));
}


float ImageLinear(const struct jaImage* image, size_t ch, float u, float v)
{
	size_t col[2];
	size_t row[2];
	float color[2];
	float dx;
	float dy;

	union {
		uint8_t* u8;
		uint16_t* u16;
		float* f32;
		void* raw;
	} component;

	component.raw = image->data;

	u = u * (float)image->width;
	v = v * (float)image->height;

	dx = u - (floorf(u) + 0.5f);
	dy = v - (floorf(v) + 0.5f);

	col[0] = jaClamp((size_t)u, 0, image->width - 1);
	row[0] = jaClamp((size_t)v, 0, image->height - 1);

	col[1] = (dx > 0.0f) ? (col[0] + 1) : (col[0] - 1);
	row[1] = (dy > 0.0f) ? (row[0] + 1) : (row[0] - 1);

	col[1] = (col[1] > image->width - 1) ? col[0] : col[1];
	row[1] = (row[1] > image->height - 1) ? row[0] : row[1];

	/*if (image->format == IMAGE_U8)
	{
	    color[0] = Mix((float)component.u8[(row[0] * image->width + col[0]) * image->channels + ch] / (float)UINT8_MAX,
	                   (float)component.u8[(row[0] * image->width + col[1]) * image->channels + ch] / (float)UINT8_MAX,
	                   fabsf(dx));
	    color[1] = Mix((float)component.u8[(row[1] * image->width + col[0]) * image->channels + ch] / (float)UINT8_MAX,
	                   (float)component.u8[(row[1] * image->width + col[1]) * image->channels + ch] / (float)UINT8_MAX,
	                   fabsf(dx));
	}
	else if (image->format == IMAGE_U16)
	{
	    color[0] =
	        Mix((float)component.u16[(row[0] * image->width + col[0]) * image->channels + ch] / (float)UINT16_MAX,
	            (float)component.u16[(row[0] * image->width + col[1]) * image->channels + ch] / (float)UINT16_MAX,
	            fabsf(dx));
	    color[1] =
	        Mix((float)component.u16[(row[1] * image->width + col[0]) * image->channels + ch] / (float)UINT16_MAX,
	            (float)component.u16[(row[1] * image->width + col[1]) * image->channels + ch] / (float)UINT16_MAX,
	            fabsf(dx));
	}
	else if (image->format == IMAGE_FLOAT)
	{*/
	color[0] = Mix(component.f32[(row[0] * image->width + col[0]) * image->channels + ch],
	               component.f32[(row[0] * image->width + col[1]) * image->channels + ch], fabsf(dx));
	color[1] = Mix(component.f32[(row[1] * image->width + col[0]) * image->channels + ch],
	               component.f32[(row[1] * image->width + col[1]) * image->channels + ch], fabsf(dx));
	/*}
	else
	{
	    color[0] = -1.0f;
	    color[1] = -1.0f;
	}*/

	return Mix(color[0], color[1], fabsf(dy));
}


/*struct jaImage* ImageUpScale(const struct jaImage* input, size_t new_width, size_t new_height, struct jaStatus* st)
{
    union {
        uint8_t* u8;
        uint16_t* u16;
        float* f32;
        void* raw;
    } component;

    float value = 0.0f;
    struct jaImage* output = jaImageCreate(input->format, new_width, new_height, input->channels);

    if (output == NULL)
    {
        jaStatusSet(st, "ImageUpScale", STATUS_MEMORY_ERROR, NULL);
        return NULL;
    }

    component.raw = output->data;

    if (output->format == IMAGE_U8)
    {
        for (size_t row = 0; row < output->height; row++)
            for (size_t col = 0; col < output->width; col++)
                for (size_t ch = 0; ch < output->channels; ch++)
                {
                    value =
                        ImageLinear(input, ch, (float)col / (float)output->width, (float)row / (float)output->height);

                    value *= UINT8_MAX;
                    component.u8[(row * output->width + col) * output->channels + ch] = (uint8_t)value;
                }
    }
    else if (output->format == IMAGE_U16)
    {
        for (size_t row = 0; row < output->height; row++)
            for (size_t col = 0; col < output->width; col++)
                for (size_t ch = 0; ch < output->channels; ch++)
                {
                    value =
                        ImageLinear(input, ch, (float)col / (float)output->width, (float)row / (float)output->height);

                    value *= UINT16_MAX;
                    component.u16[(row * output->width + col) * output->channels + ch] = (uint16_t)value;
                }
    }
    else if (output->format == IMAGE_FLOAT)
    {
        for (size_t row = 0; row < output->height; row++)
            for (size_t col = 0; col < output->width; col++)
                for (size_t ch = 0; ch < output->channels; ch++)
                {
                    value =
                        ImageLinear(input, ch, (float)col / (float)output->width, (float)row / (float)output->height);

                    component.f32[(row * output->width + col) * output->channels + ch] = value;
                }
    }

    return output;
}*/


struct jaVector3 callback(float x, float y, const struct jaImage* height, const struct jaImage* normal)
{
	struct jaVector3 grass = {0.52f, 0.56f, 0.30f};
	struct jaVector3 snow = {0.90f, 0.90f, 0.85f};

	struct jaVector3 cliff1 = {0.76f, 0.67f, 0.56f};
	struct jaVector3 cliff2 = {0.44f, 0.43f, 0.42f};

	float factor;

	// Plains
	factor = Smoothstep(0.25f, 0.75f, ImageLinear(height, 0, x, y));
	struct jaVector3 plains = MixVector3(grass, snow, factor);

	// Cliffs
	factor = Smoothstep(0.0f, 0.75f, ImageLinear(height, 0, x, y));
	struct jaVector3 cliffs = MixVector3(cliff1, cliff2, factor);

	// Combine
	float slope = sqrtf(powf(ImageLinear(normal, 0, x, y), 2.0f) + powf(ImageLinear(normal, 1, x, y), 2.0f));
	struct jaVector3 ret = MixVector3(plains, cliffs, jaClamp(BrightnessContrast(slope, 8.0f, 20.0f), 0.0f, 1.0f));

	// return (struct jaVector3){ImageLinear(normal, 0, x, y), ImageLinear(normal, 1, x, y), ImageLinear(normal, 2, x,
	// y)}; return (struct jaVector3){ImageLinear(height, 0, x, y), ImageLinear(height, 0, x, y), ImageLinear(height, 0,
	// x, y)};
	return ret;
}


struct jaImage* Normalmap(const struct jaImage* heightmap, float normal_scale, struct jaStatus* st)
{
	// https://en.wikipedia.org/wiki/Sobel_operator
	// https://en.wikipedia.org/wiki/Image_derivatives
	// https://en.wikipedia.org/wiki/Kernel_(image_processing)#Convolution

	// TODO, check that the heightmap has only 1 channel

	struct ProgressState s;
	float i = 0.0f;

	struct jaVector3* normal;
	struct jaImage* image = jaImageCreate(IMAGE_FLOAT, heightmap->width, heightmap->height, 3);

	if (image == NULL)
	{
		jaStatusSet(st, "NormalMap", STATUS_MEMORY_ERROR, NULL);
		return NULL;
	}

	ProgressInit(&s);
	normal = image->data;

	for (size_t row = 0; row < image->height; row++)
	{
		for (size_t col = 0; col < image->width; col++)
		{
			normal[row * image->width + col].x =
			    1.0f * (ImageNearCR(heightmap, 0, col - 1, row - 1) - ImageNearCR(heightmap, 0, col + 1, row - 1)) +
			    2.0f * (ImageNearCR(heightmap, 0, col - 1, row + 0) - ImageNearCR(heightmap, 0, col + 1, row + 0)) +
			    1.0f * (ImageNearCR(heightmap, 0, col - 1, row + 1) - ImageNearCR(heightmap, 0, col + 1, row + 1));

			normal[row * image->width + col].y =
			    1.0f * (ImageNearCR(heightmap, 0, col - 1, row - 1) - ImageNearCR(heightmap, 0, col - 1, row + 1)) +
			    2.0f * (ImageNearCR(heightmap, 0, col + 0, row - 1) - ImageNearCR(heightmap, 0, col + 0, row + 1)) +
			    1.0f * (ImageNearCR(heightmap, 0, col + 1, row - 1) - ImageNearCR(heightmap, 0, col + 1, row + 1));

			normal[row * image->width + col].x *= normal_scale;
			normal[row * image->width + col].y *= normal_scale;
			normal[row * image->width + col].z = 1.0f;

			normal[(row * image->width + col)] = jaVector3Normalize(normal[row * image->width + col]);

			i += 1.0f;
		}

		ProgressPrint(&s, "   ", i / (float)(image->width * image->height) * 100.f);
	}

	ProgressStop(&s, "   ");

	return image;
}


struct jaImage* ImageToFloat(struct jaImage* input, struct jaStatus* st)
{
	union {
		uint8_t* u8;
		uint16_t* u16;
		void* raw;
	} in_component;

	float* out_component = NULL;
	struct jaImage* output = jaImageCreate(IMAGE_FLOAT, input->width, input->height, input->channels);

	if (output == NULL)
	{
		jaStatusSet(st, "ImageToFloat", STATUS_MEMORY_ERROR, NULL);
		jaImageDelete(input);
		return NULL;
	}

	out_component = output->data;
	in_component.raw = input->data;

	if (input->format == IMAGE_U8)
	{
		for (size_t row = 0; row < input->height; row++)
			for (size_t col = 0; col < input->width; col++)
				for (size_t ch = 0; ch < input->channels; ch++)
				{
					out_component[(row * input->width + col) * input->channels + ch] =
					    (float)in_component.u8[(row * input->width + col) * input->channels + ch] / (float)UINT8_MAX;
				}
	}
	else if (input->format == IMAGE_U16)
	{
		for (size_t row = 0; row < input->height; row++)
			for (size_t col = 0; col < input->width; col++)
				for (size_t ch = 0; ch < input->channels; ch++)
				{
					out_component[(row * input->width + col) * input->channels + ch] =
					    (float)in_component.u16[(row * input->width + col) * input->channels + ch] / (float)UINT16_MAX;
				}
	}

	jaImageDelete(input);
	return output;
}


int BakeryTool(const char* heightmap_filename, const char* output_filename, size_t width, size_t height,
               struct jaStatus* st)
{
	struct ProgressState s;

	struct jaImage* texure = NULL;
	struct jaImage* heightmap = NULL;
	struct jaImage* normalmap = NULL;

	struct jaVector3 value;
	uint8_t* component;

	float i = 0.0f;

	if ((texure = jaImageCreate(IMAGE_U8, width, height, 3)) == NULL)
	{
		jaStatusSet(st, "BakeryTool", STATUS_MEMORY_ERROR, NULL);
		return 1;
	}

	printf(" - Loading heightmap...\n");

	if ((heightmap = jaImageLoad(heightmap_filename, st)) == NULL)
		goto return_failure;

	if ((heightmap = ImageToFloat(heightmap, st)) == NULL)
		goto return_failure;

	printf(" - Generating normalmap...\n");

	if ((normalmap = Normalmap(heightmap, 2.0f, st)) == NULL)
	{
		jaStatusSet(st, "BakeryTool", STATUS_MEMORY_ERROR, NULL);
		goto return_failure;
	}

	printf(" - Generating texture...\n");
	ProgressInit(&s);

	component = texure->data;

	for (size_t row = 0; row < texure->height; row++)
	{
		for (size_t col = 0; col < texure->width; col++)
		{
			value =
			    callback((float)col / (float)texure->width, (float)row / (float)texure->height, heightmap, normalmap);

			component[(row * texure->width + col) * texure->channels + 0] =
			    (uint8_t)(jaClamp(value.x, 0.0f, 1.0f) * (float)UINT8_MAX);

			component[(row * texure->width + col) * texure->channels + 1] =
			    (uint8_t)(jaClamp(value.y, 0.0f, 1.0f) * (float)UINT8_MAX);

			component[(row * texure->width + col) * texure->channels + 2] =
			    (uint8_t)(jaClamp(value.z, 0.0f, 1.0f) * (float)UINT8_MAX);

			i += 1.0f;
		}

		ProgressPrint(&s, "   ", i / (float)(texure->width * texure->height) * 100.f);
	}

	ProgressStop(&s, "   ");

	printf(" - Saving texture...\n");

	if (jaImageSaveSgi(texure, output_filename, st) != 0)
		goto return_failure;

	// Bye!
	jaImageDelete(heightmap);
	jaImageDelete(normalmap);
	jaImageDelete(texure);
	return 0;

return_failure:
	if (heightmap != NULL)
		jaImageDelete(heightmap);
	if (normalmap != NULL)
		jaImageDelete(normalmap);
	if (texure != NULL)
		jaImageDelete(texure);
	return 1;
}


#define NAME "Bake(ry) tool, Nara SDK"
#define NAME_SHORT "Bakery tool"


int main()
{
	struct jaStatus st = {0};
	printf("%s\n", NAME);

	if (BakeryTool("./heightmap.sgi", "./output.sgi", 4096, 4096, &st) != 0)
		goto return_failure;

	// Bye!
	return EXIT_SUCCESS;

return_failure:
	jaStatusPrint(NAME_SHORT, st);
	return EXIT_FAILURE;
}
