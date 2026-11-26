#ifndef __CONVEX_POLYGON__
#define __CONVEX_POLYGON__

#include "vsdk/toolkit/skin/RpkAxisAlignedBoundingBox.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

/**
A structure describing polygons. Only used for shaft culling for the moment.

Note this is not able to represent a general polygon, just a convex polygon with
MAXIMUM_VERTICES_PER_PATCH or less (namely, triangles and quads only)
*/
class ConvexPolygon {
  public:
    Vector3Dd normal;
    double planeConstant;
    RpkAxisAlignedBoundingBox bounds;
    Vector3Dd vertex[MAXIMUM_VERTICES_PER_PATCH];
    int numberOfVertices;
    char index;

    ConvexPolygon();
};

inline
ConvexPolygon::ConvexPolygon():
    normal(),
    planeConstant(),
    bounds(),
    vertex(),
    numberOfVertices(),
    index()
{
}

#endif
