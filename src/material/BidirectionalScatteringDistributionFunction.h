/**
A simple combination of brdf and btdf.
Handles evaluation and sampling and also
functions that relate to brdf or btdf like reflectance etc.
*/

#ifndef __BSDF__
#define __BSDF__

#include "common/ColorRgb.h"
#include "material/RayHit.h"
#include "material/xxdf.h"
#include "material/Texture.h"
#include "material/PhongBidirectionalReflectanceDistributionFunction.h"
#include "material/PhongBidirectionalTransmittanceDistributionFunction.h"

class BidirectionalScatteringDistributionFunction {
  public:
    PhongBidirectionalReflectanceDistributionFunction *brdf;
    PhongBidirectionalTransmittanceDistributionFunction *btdf;
    Texture *texture;

    explicit BidirectionalScatteringDistributionFunction(PhongBidirectionalReflectanceDistributionFunction *brdf, PhongBidirectionalTransmittanceDistributionFunction *btdf, Texture *texture);
    virtual ~BidirectionalScatteringDistributionFunction();
};

extern ColorRgb bsdfScatteredPower(const BidirectionalScatteringDistributionFunction *bsdf, RayHit *hit, const Vector3D *inDir, char flags);
extern void bsdfIndexOfRefraction(const BidirectionalScatteringDistributionFunction *bsdf, RefractionIndex *index);
extern bool bsdfShadingFrame(const BidirectionalScatteringDistributionFunction *bsdf, const RayHit *hit, const Vector3D *X, const Vector3D *Y, const Vector3D *Z);

extern ColorRgb
bsdfEvalComponents(
    const BidirectionalScatteringDistributionFunction *bsdf,
    RayHit *hit,
    const BidirectionalScatteringDistributionFunction *inBsdf,
    const BidirectionalScatteringDistributionFunction *outBsdf,
    const Vector3D *in,
    const Vector3D *out,
    char flags,
    ColorRgb *colArray);

extern Vector3D
bsdfSample(
    const BidirectionalScatteringDistributionFunction *bsdf,
    RayHit *hit,
    const BidirectionalScatteringDistributionFunction *inBsdf,
    const BidirectionalScatteringDistributionFunction *outBsdf,
    const Vector3D *in,
    int doRussianRoulette,
    BSDF_FLAGS flags,
    double x_1,
    double x_2,
    double *probabilityDensityFunction);

extern void
bsdfEvalPdf(
    const BidirectionalScatteringDistributionFunction *bsdf,
    RayHit *hit,
    const BidirectionalScatteringDistributionFunction *inBsdf,
    const BidirectionalScatteringDistributionFunction *outBsdf,
    const Vector3D *in,
    const Vector3D *out,
    BSDF_FLAGS flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR);

extern int bsdfIsTextured(const BidirectionalScatteringDistributionFunction *bsdf);

#endif
