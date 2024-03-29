#ifndef __BSDF_METHODS__
#define __BSDF_METHODS__

#include "common/color.h"
#include "material/xxdf.h"
#include "material/hit.h"

class SPLIT_BSDF;

/**
BSDF methods: every kind of BSDF needs to have these functions implemented
*/
class BSDF_METHODS {
  public:
    int (*shadingFrame)(void *data, RayHit *hit, Vector3D *X, Vector3D *Y, Vector3D *Z);
};

#include "material/splitbsdf.h"

#endif
