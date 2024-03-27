#ifndef __BTDF_METHODS__
#define __BTDF_METHODS__

#include "common/color.h"
#include "material/xxdf.h"

/**
BTDF methods: every kind of BTDF needs to have these functions
implemented
*/
class BTDF_METHODS {
  public:
    // Returns the transmittance
    COLOR (*Transmittance)(void *data, XXDFFLAGS flags);

    // Returns the index of refraction
    void (*IndexOfRefraction)(void *data, RefractionIndex *index);

    COLOR
    (*Eval)(
        void *data,
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        Vector3D *in,
        Vector3D *out,
        Vector3D *normal,
        XXDFFLAGS flags);

    Vector3D
    (*Sample)(
        void *data,
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        Vector3D *in,
        Vector3D *normal,
        int doRussianRoulette,
        XXDFFLAGS flags,
        double x1,
        double x2,
        double *probabilityDensityFunction);

    void
    (*EvalPdf)(
        void *data,
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        Vector3D *in,
        Vector3D *out,
        Vector3D *normal,
        XXDFFLAGS flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR);
};

#endif
