#ifndef __BRDF_METHODS__
#define __BRDF_METHODS__

#include <cstdio>

#include "common/color.h"
#include "material/xxdf.h"
#include "material/hit.h"

/**
BRDF methods: every kind of BRDF needs to have these functions
implemented
*/
class BRDF_METHODS {
  public:
    // Returns the reflectance
    COLOR (*Reflectance)(void *data, XXDFFLAGS flags);

    // Eval brdf
    COLOR (*Eval)(void *data, Vector3D *in, Vector3D *out, Vector3D *normal, XXDFFLAGS flags);

    Vector3D (*Sample)(void *data, Vector3D *in,
                       Vector3D *normal, int doRussianRoulette,
                       XXDFFLAGS flags, double x_1, double x_2,
                       double *probabilityDensityFunction);

    void (*EvalPdf)(void *data, Vector3D *in,
                    Vector3D *out, Vector3D *normal,
                    XXDFFLAGS flags, double *pdf, double *pdfRR);
};

#endif
