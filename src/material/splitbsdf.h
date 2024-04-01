#ifndef __SPLIT_BSDF__
#define __SPLIT_BSDF__

#include "material/texture.h"
#include "material/brdf.h"
#include "material/btdf.h"
#include "material/hit.h"

class SPLIT_BSDF {
  public:
    BRDF *brdf;
    BTDF *btdf;
    TEXTURE *texture;
};

class BSDF;

extern SPLIT_BSDF *
splitBsdfCreate(BRDF *brdf, BTDF *btdf, TEXTURE *texture);

extern COLOR
splitBsdfScatteredPower(SPLIT_BSDF *bsdf, RayHit *hit, Vector3D * /*in*/, char flags);

extern int
splitBsdfIsTextured(SPLIT_BSDF *bsdf);

extern COLOR
splitBsdfEval(
    SPLIT_BSDF *bsdf,
    RayHit *hit,
    BSDF *inBsdf,
    BSDF *outBsdf,
    Vector3D *in,
    Vector3D *out,
    BSDF_FLAGS flags);

extern void
splitBsdfIndexOfRefraction(SPLIT_BSDF *bsdf, RefractionIndex *index);

extern Vector3D
splitBsdfSample(
    SPLIT_BSDF *bsdf,
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
    SPLIT_BSDF *bsdf,
    RayHit *hit,
    BSDF *inBsdf,
    BSDF *outBsdf,
    Vector3D *in,
    Vector3D *out,
    BSDF_FLAGS flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR);

#include "material/bsdf.h"

#endif
