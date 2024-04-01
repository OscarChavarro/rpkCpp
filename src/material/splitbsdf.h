#ifndef __SPLIT_BSDF__
#define __SPLIT_BSDF__

#include "material/texture.h"
#include "material/brdf.h"
#include "material/btdf.h"
#include "material/hit.h"
#include "material/bsdf.h"

extern COLOR
splitBsdfScatteredPower(BSDF *bsdf, RayHit *hit, char flags);

extern int
splitBsdfIsTextured(BSDF *bsdf);

extern COLOR
splitBsdfEval(
    BSDF *bsdf,
    RayHit *hit,
    BSDF *inBsdf,
    BSDF *outBsdf,
    Vector3D *in,
    Vector3D *out,
    BSDF_FLAGS flags);

extern void
splitBsdfIndexOfRefraction(BSDF *bsdf, RefractionIndex *index);

extern Vector3D
splitBsdfSample(
    BSDF *bsdf,
    RayHit *hit,
    BSDF *inBsdf,
    BSDF *outBsdf,
    Vector3D *in,
    int doRussianRoulette,
    BSDF_FLAGS flags,
    double x1,
    double x2,
    double *probabilityDensityFunction);

extern void
splitBsdfEvalPdf(
    BSDF *bsdf,
    RayHit *hit,
    BSDF *inBsdf,
    BSDF *outBsdf,
    Vector3D *in,
    Vector3D *out,
    BSDF_FLAGS flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR);

#endif
