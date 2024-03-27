/**
A simple combination of brdf and btdf.
Handles evaluation and sampling and also
functions that relate to brdf or btdf like reflectance etc.

All new code should be using BSDF's since this is more general.
You should NEVER access brdf or btdf data directly from a
bsdf. Since new models may not make the same distinction for
light scattering.
*/

#ifndef __BSDF__
#define __BSDF__

#include "material/bsdf_methods.h"

class BSDF {
  public:
    void *data;
    BSDF_METHODS *methods;
};

extern BSDF *bsdfCreate(void *data, BSDF_METHODS *methods);

extern COLOR bsdfScatteredPower(BSDF *bsdf, RayHit *hit, Vector3D *dir, BSDF_FLAGS flags);

/**
Returns the reflectance of hte BSDF according to the flags
*/
inline COLOR
bsdfReflectance(BSDF *bsdf, RayHit *hit, Vector3D *dir, int xxflags) {
    return bsdfScatteredPower(bsdf, hit, dir, SET_BRDF_FLAGS(xxflags));
}

inline COLOR
bsdfSpecularReflectance(BSDF *bsdf, RayHit *hit, Vector3D *dir) {
    return bsdfReflectance((bsdf), hit, dir, SPECULAR_COMPONENT);
}

/**
Returns the transmittance of the BSDF
*/
inline COLOR
bsdfTransmittance(BSDF *bsdf, RayHit *hit, Vector3D *dir, int xxflags) {
    return bsdfScatteredPower(bsdf, hit, dir, SET_BTDF_FLAGS(xxflags));
}

inline COLOR
bsdfSpecularTransmittance(BSDF *bsdf, RayHit *hit, Vector3D *dir) {
    return bsdfTransmittance((bsdf), hit, dir, SPECULAR_COMPONENT);
}

extern void bsdfIndexOfRefraction(BSDF *bsdf, RefractionIndex *index);
extern int bsdfShadingFrame(BSDF *bsdf, RayHit *hit, Vector3D *X, Vector3D *Y, Vector3D *Z);

/**
BSDF Evaluation functions
*/

extern COLOR
bsdfEval(
    BSDF *bsdf,
    RayHit *hit,
    BSDF *inBsdf,
    BSDF *outBsdf,
    Vector3D *in,
    Vector3D *out,
    BSDF_FLAGS flags);

extern COLOR
bsdfEvalComponents(
    BSDF *bsdf,
    RayHit *hit,
    BSDF *inBsdf,
    BSDF *outBsdf,
    Vector3D *in,
    Vector3D *out,
    BSDF_FLAGS flags,
    COLOR *colArray);

extern Vector3D
bsdfSample(
    BSDF *bsdf,
    RayHit *hit,
    BSDF *inBsdf,
    BSDF *outBsdf,
    Vector3D *in,
    int doRussianRoulette,
    BSDF_FLAGS flags,
    double x_1,
    double x_2,
    double *probabilityDensityFunction);

extern void
bsdfEvalPdf(
    BSDF *bsdf,
    RayHit *hit,
    BSDF *inBsdf,
    BSDF *outBsdf,
    Vector3D *in,
    Vector3D *out,
    BSDF_FLAGS flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR);

extern int bsdfIsTextured(BSDF *bsdf);

#endif
