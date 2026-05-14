/**
A simple combination of brdf and btdf.
Handles evaluation and sampling and also
functions that relate to brdf or btdf like reflectance etc.
*/

#ifndef PHONG_BIDIRECTIONAL_SCATTERING_DISTRIBUTION_FUNCTION__
#define PHONG_BIDIRECTIONAL_SCATTERING_DISTRIBUTION_FUNCTION__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "RendererConfiguration.h"
#include "vsdk/toolkit/material/PhongBidirectionalReflectanceDistributionFunction.h"
#include "vsdk/toolkit/material/PhongBidirectionalTransmittanceDistributionFunction.h"
#include "vsdk/toolkit/material/ShadingContext.h"
#include "vsdk/toolkit/material/Texture.h"
#include "vsdk/toolkit/material/Xxdf.h"

#ifdef RAYTRACING_ENABLED
    #include "vsdk/toolkit/material/SplitBSDFSamplingMode.h"
#endif

class PhongBidirectionalScatteringDistributionFunction {
  private:
    static constexpr int TEXTURED_COMPONENT = BRDF_DIFFUSE_COMPONENT;
    PhongBidirectionalReflectanceDistributionFunction *brdf;
    PhongBidirectionalTransmittanceDistributionFunction *btdf;
    Texture *texture;

    static ColorRgb splitBsdfEvalTexture(const Texture *texture, const ShadingContext &context);

#ifdef RAYTRACING_ENABLED
    static Vector3D
    texturedScattererSample(
        const Vector3D * /*in*/,
        const Vector3D *normal,
        double x1,
        double x2,
        double *probabilityDensityFunction);

    static void
    texturedScattererEvalPdf(
        const Vector3D * /*in*/,
        const Vector3D *out,
        const Vector3D *normal,
        double *probabilityDensityFunction);

    void
    splitBsdfProbabilities(
        const ShadingContext &context,
        char flags,
        double *texture,
        double *reflection,
        double *transmission,
        char *brdfFlags,
        char *btdfFlags) const;

    static SplitBSDFSamplingMode
    splitBsdfSamplingMode(
        double texture,
        double reflection,
        double transmission,
        double *x1);

    static double
    texturedScattererEval(
        const Vector3D * /*in*/,
        const Vector3D * /*out*/,
        const Vector3D * /*normal*/);
#endif

public:
    explicit PhongBidirectionalScatteringDistributionFunction(PhongBidirectionalReflectanceDistributionFunction *brdf, PhongBidirectionalTransmittanceDistributionFunction *btdf, Texture *texture);
    virtual ~PhongBidirectionalScatteringDistributionFunction();

    const PhongBidirectionalReflectanceDistributionFunction *getBrdf() const;
    const PhongBidirectionalTransmittanceDistributionFunction *getBtdf() const;
    const Texture *getTexture() const;

    static bool bsdfShadingFrame(
        const ShadingContext &context,
        const Vector3D *X,
        const Vector3D *Y,
        const Vector3D *Z);

    ColorRgb splitBsdfScatteredPower(const ShadingContext &context, char flags) const;
    bool splitBsdfIsTextured() const;

#ifdef RAYTRACING_ENABLED
    void indexOfRefraction(RefractionIndex *index) const;

    Vector3D
    sample(
        const ShadingContext &context,
        const PhongBidirectionalScatteringDistributionFunction *inBsdf,
        const PhongBidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3D *in,
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
        const Vector3D *in,
        const Vector3D *out,
        char flags) const;

    void
    evaluateProbabilityDensityFunction(
        const ShadingContext &context,
        const PhongBidirectionalScatteringDistributionFunction *inBsdf,
        const PhongBidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        char flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR) const;

    ColorRgb
    bsdfEvalComponents(
        const ShadingContext &context,
        const PhongBidirectionalScatteringDistributionFunction *inBsdf,
        const PhongBidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        char flags,
        ColorRgb *colArray) const;

#endif

};

inline const PhongBidirectionalReflectanceDistributionFunction *
PhongBidirectionalScatteringDistributionFunction::getBrdf() const {
    return brdf;
}

inline const PhongBidirectionalTransmittanceDistributionFunction *
PhongBidirectionalScatteringDistributionFunction::getBtdf() const {
    return btdf;
}

inline const Texture *
PhongBidirectionalScatteringDistributionFunction::getTexture() const {
    return texture;
}

#endif
