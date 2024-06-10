/**
A simple combination of brdf and btdf.
Handles evaluation and sampling and also
functions that relate to brdf or btdf like reflectance etc.
*/

#ifndef __PHONG_BIDIRECTIONAL_SCATTERING_DISTRIBUTION_FUNCTION__
#define __PHONG_BIDIRECTIONAL_SCATTERING_DISTRIBUTION_FUNCTION__

#include "common/RenderOptions.h"
#include "common/ColorRgb.h"
#include "material/RayHit.h"
#include "material/xxdf.h"
#include "material/Texture.h"
#include "material/PhongBidirectionalReflectanceDistributionFunction.h"
#include "material/PhongBidirectionalTransmittanceDistributionFunction.h"

#ifdef RAYTRACING_ENABLED
    #include "material/SplitBSDFSamplingMode.h"
#endif

class PhongBidirectionalScatteringDistributionFunction {
  private:
    PhongBidirectionalReflectanceDistributionFunction *brdf;
    PhongBidirectionalTransmittanceDistributionFunction *btdf;
    Texture *texture;

    static ColorRgb splitBsdfEvalTexture(const Texture *texture,  RayHit *hit);

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
        RayHit *hit,
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

    static bool bsdfShadingFrame(
        const RayHit *hit,
        const Vector3D *X,
        const Vector3D *Y,
        const Vector3D *Z);

    ColorRgb splitBsdfScatteredPower(RayHit *hit, char flags) const;
    int splitBsdfIsTextured() const;

#ifdef RAYTRACING_ENABLED
    void indexOfRefraction(RefractionIndex *index) const;

    Vector3D
    sample(
        RayHit *hit,
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
        RayHit *hit,
        const PhongBidirectionalScatteringDistributionFunction *inBsdf,
        const PhongBidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        char flags) const;

    void
    evaluateProbabilityDensityFunction(
        RayHit *hit,
        const PhongBidirectionalScatteringDistributionFunction *inBsdf,
        const PhongBidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        char flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR) const;

    ColorRgb
    bsdfEvalComponents(
        RayHit *hit,
        const PhongBidirectionalScatteringDistributionFunction *inBsdf,
        const PhongBidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        char flags,
        ColorRgb *colArray) const;
#endif

};

#endif
