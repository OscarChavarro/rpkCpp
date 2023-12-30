/*
 * screeniterate.H : iterates over all pixels of the screen calling
 *   a callback function. This function should return the color for
 *   that pixel. This color is transformed into RGB and displayed.
 *   Several functions are provided for different iterating schemes
 */

#ifndef _SCREENITERATE_H_
#define _SCREENITERATE_H_

#include "material/color.h"

typedef COLOR(*SCREENITERATECALLBACK)(int, int, void *);

void ScreenIterateSequential(SCREENITERATECALLBACK callback, void *data);

void ScreenIterateProgressive(SCREENITERATECALLBACK callback,
                              void *data);

#endif
