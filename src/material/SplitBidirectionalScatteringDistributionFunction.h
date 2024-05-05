#ifndef __SPLIT_BSDF__
#define __SPLIT_BSDF__

#include "material/Texture.h"
#include "material/btdf.h"
#include "material/RayHit.h"
#include "material/bsdf.h"

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

class SplitBidirectionalScatteringDistributionFunction {
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
        const BidirectionalScatteringDistributionFunction *bsdf,
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
    static ColorRgb splitBsdfScatteredPower(const BidirectionalScatteringDistributionFunction *bsdf, RayHit *hit, char flags);
    static int splitBsdfIsTextured(const BidirectionalScatteringDistributionFunction *bsdf);

    static ColorRgb
    evaluate(
        const BidirectionalScatteringDistributionFunction *bsdf,
        RayHit *hit,
        const BidirectionalScatteringDistributionFunction *inBsdf,
        const BidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        char flags);

    static void indexOfRefraction(const BidirectionalScatteringDistributionFunction *bsdf, RefractionIndex *index);

    static Vector3D
    sample(
        const BidirectionalScatteringDistributionFunction *bsdf,
        RayHit *hit,
        const BidirectionalScatteringDistributionFunction *inBsdf,
        const BidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3D *in,
        int doRussianRoulette,
        char flags,
        double x1,
        double x2,
        double *probabilityDensityFunction);

    static void
    evaluateProbabilityDensityFunction(
        const BidirectionalScatteringDistributionFunction *bsdf,
        RayHit *hit,
        const BidirectionalScatteringDistributionFunction *inBsdf,
        const BidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        char flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR);
};

#endif
