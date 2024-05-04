#ifndef __SPLIT_BSDF__
#define __SPLIT_BSDF__

#include "material/Texture.h"
#include "material/brdf.h"
#include "material/btdf.h"
#include "material/RayHit.h"
#include "material/bsdf.h"

extern ColorRgb
splitBsdfScatteredPower(const BSDF *bsdf, RayHit *hit, char flags);

extern int
splitBsdfIsTextured(const BSDF *bsdf);

extern ColorRgb
splitBsdfEval(
    const BSDF *bsdf,
    RayHit *hit,
    const BSDF *inBsdf,
    const BSDF *outBsdf,
    const Vector3D *in,
    const Vector3D *out,
    char flags);

extern void
splitBsdfIndexOfRefraction(const BSDF *bsdf, RefractionIndex *index);

extern Vector3D
splitBsdfSample(
    const BSDF *bsdf,
    RayHit *hit,
    const BSDF *inBsdf,
    const BSDF *outBsdf,
    const Vector3D *in,
    int doRussianRoulette,
    char flags,
    double x1,
    double x2,
    double *probabilityDensityFunction);

extern void
splitBsdfEvalPdf(
    const BSDF *bsdf,
    RayHit *hit,
    const BSDF *inBsdf,
    const BSDF *outBsdf,
    const Vector3D *in,
    const Vector3D *out,
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR);

#endif
