#ifndef POLYGON__
#define POLYGON__

#include "skin/AxisAlignedBoundingBox.h"
#include "environment/geometry/elements/Patch.h"

/**
A structure describing polygons. Only used for shaft culling for the moment.

Note this is not able to represent a general polygon, just a convex polygon with
MAXIMUM_VERTICES_PER_PATCH or less (namely, triangles and quads only)
*/
class Polygon {
  public:
    Vector3D normal;
    float planeConstant;
    AxisAlignedBoundingBox bounds;
    Vector3D vertex[MAXIMUM_VERTICES_PER_PATCH];
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
