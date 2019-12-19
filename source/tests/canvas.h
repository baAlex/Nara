/*-----------------------------

 [canvas.h]
 - Alexander Brandt 2019
-----------------------------*/

#ifndef CANVAS_H
#define CANVAS_H

	#include <stdlib.h>
	#include "japan-vector.h"

	struct Canvas;

	struct Canvas* CanvasCreate(size_t width, size_t height);
	void CanvasDelete(struct Canvas* canvas);
	void CanvasSave(const struct Canvas* canvas, const char* filename);

	void CanvasSetOffset(struct Canvas* canvas, struct jaVector2 offset);
	void CanvasSetColor(struct Canvas* canvas, struct jaVector3 color);
	void CanvasDrawLine(struct Canvas* canvas, struct jaVector2 a, struct jaVector2 b);

	struct jaVector2 CanvasGetOffset(const struct Canvas* canvas);
	struct jaVector3 CanvasGetColor(const struct Canvas* canvas);

#endif
