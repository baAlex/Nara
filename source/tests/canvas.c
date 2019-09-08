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

 [canvas.c]
 - Alexander Brandt 2019
-----------------------------*/

#include "canvas.h"
#include "buffer.h"
#include <stdio.h>


enum CanvasInstructionType
{
	SET_OFFSET,
	SET_COLOR,
	DRAW_LINE
};

struct CanvasInstruction
{
	enum CanvasInstructionType type;

	union {
		struct Vector3 color;
		struct Vector2 offset;

		// DRAW_LINE
		struct
		{
			struct Vector2 a;
			struct Vector2 b;
		};
	};
};

struct Canvas
{
	size_t width;
	size_t height;

	struct Vector2 state_offset;
	struct Vector3 state_color;

	struct Buffer instructions;
	size_t instructions_no;
};


inline struct Vector2 CanvasGetOffset(const struct Canvas* canvas)
{
	return canvas->state_offset;
}

inline struct Vector3 CanvasGetColor(const struct Canvas* canvas)
{
	return canvas->state_color;
}


/*-----------------------------

 CanvasCreate()
-----------------------------*/
inline struct Canvas* CanvasCreate(size_t width, size_t height)
{
	struct Canvas* canvas = NULL;

	if ((canvas = calloc(1, sizeof(struct Canvas))) != NULL)
	{
		canvas->width = width;
		canvas->height = height;
	}

	return canvas;
}


/*-----------------------------

 CanvasDelete()
-----------------------------*/
inline void CanvasDelete(struct Canvas* canvas)
{
	BufferClean(&canvas->instructions);
	free(canvas);
}


/*-----------------------------

 CanvasSave()
-----------------------------*/
void CanvasSave(const struct Canvas* canvas, const char* filename)
{
	// https://gitlab.gnome.org/GNOME/gimp/issues/2561

	FILE* fp = fopen(filename, "w");
	struct CanvasInstruction* instruction = canvas->instructions.data;

	struct Vector2 state_offset = {0};
	struct Vector3i state_color = {0};

	if (fp != NULL)
	{
		fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
		fprintf(fp, "<svg width=\"%zupx\" height=\"%zupx\" xmlns=\"http://www.w3.org/2000/svg\">\n", canvas->width,
		        canvas->height);

		fprintf(fp, "<rect x=\"-1\" y=\"-1\" width=\"%zu\" height=\"%zu\" fill=\"black\"/>\n", canvas->width + 2,
		        canvas->height + 2);

		for (size_t i = 0; i < canvas->instructions_no; i++)
		{
			switch (instruction->type)
			{
			case SET_OFFSET: state_offset = instruction->offset; break;

			case SET_COLOR:
				state_color.x = (int)((float)instruction->color.x * 255.0);
				state_color.y = (int)((float)instruction->color.y * 255.0);
				state_color.z = (int)((float)instruction->color.z * 255.0);
				break;

			case DRAW_LINE:
				fprintf(fp, "<line x1=\"%.02f\" y1=\"%.02f\" x2=\"%.02f\" y2=\"%.02f\" stroke=\"#%02X%02X%02X\"/>\n",
				        instruction->a.x + state_offset.x, instruction->a.y + state_offset.y,
				        instruction->b.x + state_offset.x, instruction->b.y + state_offset.y, state_color.x,
				        state_color.y, state_color.z);
				break;
			}

			instruction++;
		}

		fprintf(fp, "</svg>\n");
		fclose(fp);
	}
}


/*-----------------------------

 CanvasSetOffset()
-----------------------------*/
void CanvasSetOffset(struct Canvas* canvas, struct Vector2 offset)
{
	if (BufferResize(&canvas->instructions, (canvas->instructions_no + 1) * sizeof(struct CanvasInstruction)) != NULL)
	{
		canvas->instructions_no++;
		canvas->state_offset = offset;

		struct CanvasInstruction* instruction = canvas->instructions.data;
		instruction += (canvas->instructions_no - 1);

		instruction->type = SET_OFFSET;
		instruction->offset = offset;
	}
}


/*-----------------------------

 CanvasSetColor()
-----------------------------*/
void CanvasSetColor(struct Canvas* canvas, struct Vector3 color)
{
	if (BufferResize(&canvas->instructions, (canvas->instructions_no + 1) * sizeof(struct CanvasInstruction)) != NULL)
	{
		canvas->instructions_no++;
		canvas->state_color = color;

		struct CanvasInstruction* instruction = canvas->instructions.data;
		instruction += (canvas->instructions_no - 1);

		instruction->type = SET_COLOR;
		instruction->color = color;
	}
}

/*-----------------------------

 CanvasDrawLine()
-----------------------------*/
void CanvasDrawLine(struct Canvas* canvas, struct Vector2 a, struct Vector2 b)
{
	if (BufferResize(&canvas->instructions, (canvas->instructions_no + 1) * sizeof(struct CanvasInstruction)) != NULL)
	{
		canvas->instructions_no++;

		struct CanvasInstruction* instruction = canvas->instructions.data;
		instruction += (canvas->instructions_no - 1);

		instruction->type = DRAW_LINE;
		instruction->a = a;
		instruction->b = b;
	}
}
