/**
Iterates over all pixels of the screen calling
a callback function. This function should return the color for
that pixel. This color is transformed into RGB and displayed.
Several functions are provided for different iterating schemes
*/

#ifndef __SCREEN_ITERATE__
#define __SCREEN_ITERATE__

#include "common/ColorRgb.h"
#include "scene/Background.h"

typedef ColorRgb(*SCREEN_ITERATE_CALLBACK)(Background *, int, int, void *);

void ScreenIterateSequential(SCREEN_ITERATE_CALLBACK callback, void *data);

void ScreenIterateProgressive(SCREEN_ITERATE_CALLBACK callback,
                              void *data);

#endif
