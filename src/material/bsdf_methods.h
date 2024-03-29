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
    // Returns the scattered power (diffuse/glossy/specular
    // reflectance and/or transmittance) according to flags
    COLOR (*scatteredPower)(SPLIT_BSDF *data, RayHit *hit, Vector3D *in, BSDF_FLAGS flags);

    int (*isTextured)(void *data);

    // Returns the index of refraction
    void (*indexOfRefraction)(void *data, RefractionIndex *index);

    // void *incomingBsdf should be BSDF *incomingBsdf
    COLOR (*evaluate)(void *data, RayHit *hit, void *inBsdf, void *outBsdf, Vector3D *in, Vector3D *out, BSDF_FLAGS flags);

    Vector3D (*sample)(
        void *data,
        RayHit *hit,
        void *inBsdf,
        void *outBsdf,
        Vector3D *in,
        int doRussianRoulette,
        BSDF_FLAGS flags,
        double x1,
        double x2,
        double *probabilityDensityFunction);

    void (*evalPdf)(
        void *data,
        RayHit *hit,
        void *inBsdf,
        void *outBsdf,
        Vector3D *in,
        Vector3D *out,
        BSDF_FLAGS flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR);

    // Constructs shading frame at hit point. Returns TRUE if successful and
    // FALSE if not. X and Y may be null pointers
    int (*shadingFrame)(void *data, RayHit *hit, Vector3D *X, Vector3D *Y, Vector3D *Z);
};

#include "material/splitbsdf.h"

#endif
