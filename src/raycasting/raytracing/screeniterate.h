/**
Iterates over all pixels of the screen calling
a callback function. This function should return the color for
that pixel. This color is transformed into RGB and displayed.
Several functions are provided for different iterating schemes
*/

#ifndef __SCREEN_ITERATE__
#define __SCREEN_ITERATE__

#include "common/color.h"

typedef COLOR(*SCREENITERATECALLBACK)(int, int, void *);

void ScreenIterateSequential(SCREENITERATECALLBACK callback, void *data);

void ScreenIterateProgressive(SCREENITERATECALLBACK callback,
                              void *data);

#endif
