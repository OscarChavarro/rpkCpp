/**
A simple combination of brdf and btdf.
Handles evaluation and sampling and also
functions that relate to brdf or btdf like reflectance etc.
*/

#ifndef __PHONG_BIDIRECTIONAL_SCATTERING_DISTRIBUTION_FUNCTION__
#define __PHONG_BIDIRECTIONAL_SCATTERING_DISTRIBUTION_FUNCTION__

#include "common/ColorRgb.h"
#include "material/RayHit.h"
#include "material/xxdf.h"
#include "material/Texture.h"
#include "material/PhongBidirectionalReflectanceDistributionFunction.h"
#include "material/PhongBidirectionalTransmittanceDistributionFunction.h"

/**
Selects sampling mode according to given probabilities and random number x1.
If the selected mode is not absorption, x_1 is rescaled to the interval [0, 1)
again
*/
enum SplitBSDFSamplingMode {
    SAMPLE_TEXTURE,
    SAMPLE_REFLECTION,
    SAMPLE_TRANSMISSION,
    SAMPLE_ABSORPTION
};

class PhongBidirectionalScatteringDistributionFunction {
  private:
    static ColorRgb splitBsdfEvalTexture(const Texture *texture,  RayHit *hit);

    static double
    texturedScattererEval(
        const Vector3D * /*in*/,
        const Vector3D * /*out*/,
        const Vector3D * /*normal*/);

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

    static void
    splitBsdfProbabilities(
        const PhongBidirectionalScatteringDistributionFunction *bsdf,
        RayHit *hit,
        BSDF_FLAGS flags,
        double *texture,
        double *reflection,
        double *transmission,
        char *brdfFlags,
        char *btdfFlags);

    static SplitBSDFSamplingMode
    splitBsdfSamplingMode(
        double texture,
        double reflection,
        double transmission,
        double *x1);

public:
    PhongBidirectionalReflectanceDistributionFunction *brdf;
    PhongBidirectionalTransmittanceDistributionFunction *btdf;
    Texture *texture;

    explicit PhongBidirectionalScatteringDistributionFunction(PhongBidirectionalReflectanceDistributionFunction *brdf, PhongBidirectionalTransmittanceDistributionFunction *btdf, Texture *texture);
    virtual ~PhongBidirectionalScatteringDistributionFunction();

    static bool bsdfShadingFrame(
        const RayHit *hit,
        const Vector3D *X,
        const Vector3D *Y,
        const Vector3D *Z);

    ColorRgb
    bsdfEvalComponents(
        RayHit *hit,
        const PhongBidirectionalScatteringDistributionFunction *inBsdf,
        const PhongBidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        char flags,
        ColorRgb *colArray) const;

    static ColorRgb splitBsdfScatteredPower(const PhongBidirectionalScatteringDistributionFunction *bsdf, RayHit *hit, char flags);
    static int splitBsdfIsTextured(const PhongBidirectionalScatteringDistributionFunction *bsdf);

    static ColorRgb
    evaluate(
        const PhongBidirectionalScatteringDistributionFunction *bsdf,
        RayHit *hit,
        const PhongBidirectionalScatteringDistributionFunction *inBsdf,
        const PhongBidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        char flags);

    static void indexOfRefraction(const PhongBidirectionalScatteringDistributionFunction *bsdf, RefractionIndex *index);

    static Vector3D
    sample(
        const PhongBidirectionalScatteringDistributionFunction *bsdf,
        RayHit *hit,
        const PhongBidirectionalScatteringDistributionFunction *inBsdf,
        const PhongBidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3D *in,
        int doRussianRoulette,
        char flags,
        double x1,
        double x2,
        double *probabilityDensityFunction);

    static void
    evaluateProbabilityDensityFunction(
        const PhongBidirectionalScatteringDistributionFunction *bsdf,
        RayHit *hit,
        const PhongBidirectionalScatteringDistributionFunction *inBsdf,
        const PhongBidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        char flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR);
};

#endif
