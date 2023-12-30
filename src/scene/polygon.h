#ifndef _POLYGON_H_
#define _POLYGON_H_

#include "common/bounds.h"
#include "skin/patch.h"

/* A structure describing polygons. Only used for shaftculling for the moment. */
class POLYGON {
  public:
    Vector3D normal;
    float plane_constant;
    BOUNDINGBOX bounds;
    Vector3D vertex[PATCHMAXVERTICES];
    int nrvertices, index;
};

#endif
