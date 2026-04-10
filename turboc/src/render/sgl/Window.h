#ifndef __SGL_WINDOW__
#define __SGL_WINDOW__

#include "common/VSDK.h"

// WINDOW: A DISCRETE 2-D RECTANGLE
class Window {
  public:
    int x0; // x-min and y-min
    int y0;
    int x1; // x-max and y-max (inclusive)
    int y1;
};

#endif
