/**
Bidirectional Reflectance Distribution Functions
*/

#ifndef __BRDF__
#define __BRDF__

#include "material/phong.h"

class BRDF {
  public:
    PhongBidirectionalReflectanceDistributionFunction *data;
};

extern ColorRgb brdfReflectance(const PhongBidirectionalReflectanceDistributionFunction *brdf, char flags);

/**
BRDF evaluation functions :
  Vector3D in : incoming ray direction (to patch)
  Vector3D out : reflected ray direction (from patch)
  Vector3D normal : normal vector
  char flags : flags indicating which components must be
    evaluated
*/

extern ColorRgb
brdfEval(
    const PhongBidirectionalReflectanceDistributionFunction *brdf,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    char flags);

extern Vector3D
brdfSample(
    const PhongBidirectionalReflectanceDistributionFunction *brdf,
    const Vector3D *in,
    const Vector3D *normal,
    int doRussianRoulette,
    char flags,
    double x1,
    double x2,
    double *probabilityDensityFunction);

extern void
brdfEvalPdf(
    const PhongBidirectionalReflectanceDistributionFunction *brdf,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR);

#endif
