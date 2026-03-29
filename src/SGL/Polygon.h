#ifndef __SGL_POLYGON__
#define __SGL_POLYGON__

#include "SGL/PolygonConstants.h"
#include "SGL/PolygonVertex.h"

// Note: don't put > 32 doubles in Poly_vert, or mask will overflow
class Polygon {
  public:
    int n; // Number of sides
    unsigned long mask; // Interpolation mask for vertex elems
    PolygonVertex vertices[MAXIMUM_SIDES_PER_POLYGON];
};

#endif
