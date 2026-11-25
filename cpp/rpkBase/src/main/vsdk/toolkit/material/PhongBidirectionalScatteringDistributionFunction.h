#ifndef __PHONG_BIDIRECTIONAL_SCATTERING_DISTRIBUTION_FUNCTION__
#define __PHONG_BIDIRECTIONAL_SCATTERING_DISTRIBUTION_FUNCTION__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/environment/material/PhongMaterial.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/material/ShadingContext.h"
#include "vsdk/toolkit/material/Xxdf.h"

/**
RenderPark's view of the scattering of a material (its split BSDF: the
reflection, the transmission and the texture): the evaluation, sampling and
density functions the raytracers call, with RenderPark's argument types.
Every function converts its arguments (Vector3Df to Vector3Df, the shading
context to a SurfaceShadingPoint, the media to their VITRAL materials) and
answers through the VITRAL PhongMaterial; the scattering model itself lives
in VITRAL (plan step 2b). The material and the media are the ones of the
RpkMaterials that own these views, so a view identifies its material.

Read-only after construction; does not own the PhongMaterial.
*/
class PhongBidirectionalScatteringDistributionFunction {
  private:
    const PhongMaterial *material;

    static const PhongMaterial *mediumOf(const PhongBidirectionalScatteringDistributionFunction *bsdf);

  public:
    explicit PhongBidirectionalScatteringDistributionFunction(const PhongMaterial *inMaterial);
    virtual ~PhongBidirectionalScatteringDistributionFunction();

    const PhongMaterial *getMaterial() const;

    static bool bsdfShadingFrame(
        const ShadingContext &context,
        const Vector3Df *X,
        const Vector3Df *Y,
        const Vector3Df *Z);

    ColorRgb splitBsdfScatteredPower(const ShadingContext &context, char flags) const;
    bool splitBsdfIsTextured() const;

#ifdef RAYTRACING_ENABLED
    void indexOfRefraction(RefractionIndex *index) const;

    Vector3Df
    sample(
        const ShadingContext &context,
        const PhongBidirectionalScatteringDistributionFunction *inBsdf,
        const PhongBidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3Df *in,
        int doRussianRoulette,
        char flags,
        double x1,
        double x2,
        double *probabilityDensityFunction) const;

    ColorRgb
    evaluate(
        const ShadingContext &context,
        const PhongBidirectionalScatteringDistributionFunction *inBsdf,
        const PhongBidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3Df *in,
        const Vector3Df *out,
        char flags) const;

    void
    evaluateProbabilityDensityFunction(
        const ShadingContext &context,
        const PhongBidirectionalScatteringDistributionFunction *inBsdf,
        const PhongBidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3Df *in,
        const Vector3Df *out,
        char flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR) const;

    ColorRgb
    bsdfEvalComponents(
        const ShadingContext &context,
        const PhongBidirectionalScatteringDistributionFunction *inBsdf,
        const PhongBidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3Df *in,
        const Vector3Df *out,
        char flags,
        ColorRgb *colArray) const;
#endif
};

inline const PhongMaterial *
PhongBidirectionalScatteringDistributionFunction::getMaterial() const {
    return material;
}

#endif
