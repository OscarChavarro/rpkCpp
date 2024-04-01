/**
Bidirectional Reflectance Distribution Functions
*/

#ifndef __BRDF__
#define __BRDF__

#include "material/phong.h"

class BRDF {
  public:
    PhongBiDirectionalReflectanceDistributionFunction *data;
};

extern BRDF *brdfCreate(PhongBiDirectionalReflectanceDistributionFunction *data);
extern COLOR brdfReflectance(BRDF *brdf, char flags);

/**
BRDF evaluation functions :
  Vector3D in : incoming ray direction (to patch)
  Vector3D out : reflected ray direction (from patch)
  Vector3D normal : normal vector
  char flags : flags indicating which components must be
    evaluated
*/

extern COLOR
brdfEval(
    BRDF *brdf,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    char flags);

extern Vector3D
brdfSample(
    BRDF *brdf,
    Vector3D *in,
    Vector3D *normal,
    int doRussianRoulette,
    char flags,
    double x_1,
    double x_2,
    double *probabilityDensityFunction);

extern void
brdfEvalPdf(
    BRDF *brdf,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR);

#endif
