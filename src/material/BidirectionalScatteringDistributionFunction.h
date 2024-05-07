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

    static bool bsdfShadingFrame(
        const RayHit *hit,
        const Vector3D *X,
        const Vector3D *Y,
        const Vector3D *Z);

    ColorRgb
    bsdfEvalComponents(
        RayHit *hit,
        const BidirectionalScatteringDistributionFunction *inBsdf,
        const BidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        char flags,
        ColorRgb *colArray) const;
};

#endif
