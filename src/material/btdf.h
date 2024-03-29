/**
Bidirectional Transmittance Distribution Functions
*/

#ifndef __BTDF__
#define __BTDF__

#include "scene/phong.h"

/**
Index of refraction data type. Normally when using BSDF's
this should not be needed. In C++ this would of course
be a plain complex number
*/
class BTDF {
  public:
    PHONG_BTDF *data;
};

extern BTDF *btdfCreate(PHONG_BTDF *data);
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
