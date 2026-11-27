#ifndef __POLYGON__
#define __POLYGON__

#include "vsdk/toolkit/skin/RpkAxisAlignedBoundingBox.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

/**
A structure describing polygons. Only used for shaft culling for the moment.

Note this is not able to represent a general polygon, just a convex polygon with
MAXIMUM_VERTICES_PER_PATCH or less (namely, triangles and quads only)
*/
class Polygon {
  public:
    Vector3Dd normal;
    double planeConstant;
    RpkAxisAlignedBoundingBox bounds;
    Vector3Dd vertex[MAXIMUM_VERTICES_PER_PATCH];
    int numberOfVertices;
    char index;

    Polygon();
};

inline
Polygon::Polygon():
    normal(),
    planeConstant(),
    bounds(),
    vertex(),
    numberOfVertices(),
    index()
{
}

#endif
