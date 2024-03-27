/**
Bidirectional Transmittance Distribution Functions
*/

#ifndef __BTDF__
#define __BTDF__

#include "material/btdf_methods.h"

/**
Index of refraction data type. Normally when using BSDF's
this should not be needed. In C++ this would of course
be a plain complex number
*/
class BTDF {
  public:
    void *data; // TODO SITHMASTER: Should replace void * by some on the BTDF hierarchy (i.e. PHONG_BTDF)
    BTDF_METHODS *methods;
};

extern BTDF *btdfCreate(void *data, BTDF_METHODS *methods);
extern COLOR btdfTransmittance(BTDF *btdf, XXDFFLAGS flags);
extern void btdfIndexOfRefraction(BTDF *btdf, RefractionIndex *index);

/************* BTDF Evaluation functions ****************/

/**
All components of the Btdf

Vector directions :

in : towards patch
out : from patch
normal : leaving from patch, on the incoming side.
         So in.normal < 0 !!!
*/

extern COLOR
btdfEval(
    BTDF *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    XXDFFLAGS flags);

extern Vector3D
btdfSample(
    BTDF *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    Vector3D *in,
    Vector3D *normal,
    int doRussianRoulette,
    XXDFFLAGS flags,
    double x1,
    double x2,
    double *probabilityDensityFunction);

extern void
btdfEvalPdf(
    BTDF *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    XXDFFLAGS flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR);

#endif
