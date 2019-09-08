/*-----------------------------

 [canvas.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef CANVAS_H
#define CANVAS_H

	#include <stdlib.h>
	#include "vector.h"

	struct Canvas;

	struct Canvas* CanvasCreate(size_t width, size_t height);
	void CanvasDelete(struct Canvas* canvas);
	void CanvasSave(const struct Canvas* canvas, const char* filename);

	void CanvasSetOffset(struct Canvas* canvas, struct Vector2 offset);
	void CanvasSetColor(struct Canvas* canvas, struct Vector3 color);
	void CanvasDrawLine(struct Canvas* canvas, struct Vector2 a, struct Vector2 b);

	struct Vector2 CanvasGetOffset(const struct Canvas* canvas);
	struct Vector3 CanvasGetColor(const struct Canvas* canvas);

#endif
