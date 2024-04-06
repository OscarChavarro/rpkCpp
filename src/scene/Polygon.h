#ifndef __POLYGON__
#define __POLYGON__

#include "skin/BoundingBox.h"
#include "skin/Patch.h"

/**
A structure describing polygons. Only used for shaft culling for the moment.

Note this is not able to represent a general polygon, just a convex polygon with
MAXIMUM_VERTICES_PER_PATCH or less (namely, triangles and quads only)
*/
class Polygon {
  public:
    Vector3D normal;
    float planeConstant;
    BoundingBox bounds;
    Vector3D vertex[MAXIMUM_VERTICES_PER_PATCH];
    int numberOfVertices;
    char index;

    Polygon():
        normal(),
        planeConstant(),
        bounds(),
        vertex(),
        numberOfVertices(),
        index()
    {
    }
};

#endif
