#ifndef __BRDF_METHODS__
#define __BRDF_METHODS__

#include <cstdio>

#include "common/color.h"
#include "material/xxdf.h"
#include "material/hit.h"

/**
BRDF methods: every kind of BRDF needs to have these functions implemented
*/
class BRDF_METHODS {
  public:
    // Returns the reflectance
    COLOR (*reflectance)(void *data, XXDFFLAGS flags);

    // Eval brdf
    COLOR
    (*evaluate)(
        void *data,
        Vector3D *in,
        Vector3D *out,
        Vector3D *normal,
        XXDFFLAGS flags);

    Vector3D
    (*sample)(
        void *data,
        Vector3D *in,
        Vector3D *normal,
        int doRussianRoulette,
        XXDFFLAGS flags,
        double x1,
        double x2,
        double *probabilityDensityFunction);

    void
    (*evaluatePdf)(
        void *data,
        Vector3D *in,
        Vector3D *out,
        Vector3D *normal,
        XXDFFLAGS flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR);
};

#endif
