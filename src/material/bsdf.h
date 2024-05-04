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

#include "common/ColorRgb.h"
#include "material/RayHit.h"
#include "material/xxdf.h"
#include "material/Texture.h"
#include "material/brdf.h"
#include "material/btdf.h"

class SPLIT_BSDF;

class BSDF {
  public:
    PhongBidirectionalReflectanceDistributionFunction *brdf;
    PhongBidirectionalTransmittanceDistributionFunction *btdf;
    Texture *texture;

    explicit BSDF(PhongBidirectionalReflectanceDistributionFunction *brdf, PhongBidirectionalTransmittanceDistributionFunction *btdf, Texture *texture);
    virtual ~BSDF();
};

extern ColorRgb bsdfScatteredPower(const BSDF *bsdf, RayHit *hit, const Vector3D *inDir, char flags);

/**
Returns the reflectance of hte BSDF according to the flags
*/
inline ColorRgb
bsdfReflectance(const BSDF *bsdf, RayHit *hit, const Vector3D *dir, int xxflags) {
    return bsdfScatteredPower(bsdf, hit, dir, SET_BRDF_FLAGS(xxflags));
}

inline ColorRgb
bsdfSpecularReflectance(const BSDF *bsdf, RayHit *hit, const Vector3D *dir) {
    return bsdfReflectance(bsdf, hit, dir, SPECULAR_COMPONENT);
}

/**
Returns the transmittance of the BSDF
*/
inline ColorRgb
bsdfTransmittance(const BSDF *bsdf, RayHit *hit, const Vector3D *dir, int xxflags) {
    return bsdfScatteredPower(bsdf, hit, dir, SET_BTDF_FLAGS(xxflags));
}

inline ColorRgb
bsdfSpecularTransmittance(const BSDF *bsdf, RayHit *hit, const Vector3D *dir) {
    return bsdfTransmittance(bsdf, hit, dir, SPECULAR_COMPONENT);
}

extern void bsdfIndexOfRefraction(const BSDF *bsdf, RefractionIndex *index);
extern bool bsdfShadingFrame(const BSDF *bsdf, const RayHit *hit, const Vector3D *X, const Vector3D *Y, const Vector3D *Z);

/**
BSDF Evaluation functions
*/

extern ColorRgb
bsdfEval(
    const BSDF *bsdf,
    RayHit *hit,
    const BSDF *inBsdf,
    const BSDF *outBsdf,
    const Vector3D *in,
    const Vector3D *out,
    char flags);

extern ColorRgb
bsdfEvalComponents(
    const BSDF *bsdf,
    RayHit *hit,
    const BSDF *inBsdf,
    const BSDF *outBsdf,
    const Vector3D *in,
    const Vector3D *out,
    char flags,
    ColorRgb *colArray);

extern Vector3D
bsdfSample(
    const BSDF *bsdf,
    RayHit *hit,
    const BSDF *inBsdf,
    const BSDF *outBsdf,
    const Vector3D *in,
    int doRussianRoulette,
    BSDF_FLAGS flags,
    double x_1,
    double x_2,
    double *probabilityDensityFunction);

extern void
bsdfEvalPdf(
    const BSDF *bsdf,
    RayHit *hit,
    const BSDF *inBsdf,
    const BSDF *outBsdf,
    const Vector3D *in,
    const Vector3D *out,
    BSDF_FLAGS flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR);

extern int bsdfIsTextured(const BSDF *bsdf);

#include "material/splitbsdf.h"

#endif
