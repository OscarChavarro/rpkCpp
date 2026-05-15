#ifndef __POLYGON_BOX__
#define __POLYGON_BOX__

#include "common/VSDK.h"

// A BOX (TYPICALLY IN SCREEN SPACE)
class PolygonBox {
  public:
    double x0; // Left and right
    double x1;
    double y0; // Top and bottom
    double y1;
    double z0; // Near and far
    double z1;
};

#endif
