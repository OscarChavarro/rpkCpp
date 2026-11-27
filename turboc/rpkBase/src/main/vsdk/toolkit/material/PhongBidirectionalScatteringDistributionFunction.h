#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

/**
A simple combination of brdf and btdf.
Handles evaluation and sampling and also
functions that relate to brdf or btdf like reflectance etc.
*/

#ifndef PHNG_BDRCT_SCTTR_DSTRB_FNCTN
#define PHNG_BDRCT_SCTTR_DSTRB_FNCTN

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/material/PhongBidirectionalReflectanceDistributionFunction.h"
#include "vsdk/toolkit/material/PhongBidirectionalTransmittanceDistributionFunction.h"
#include "vsdk/toolkit/material/ShadingContext.h"
#include "vsdk/toolkit/material/Texture.h"
#include "vsdk/toolkit/material/Xxdf.h"

#ifdef RAYTRACING_ENABLED
    #include "vsdk/toolkit/material/SplitBSDFSamplingMode.h"
#endif

class PhongBidirScattDistFunc {
  private:
    #define TEXTURED_COMPONENT BRDF_DIFFUSE_COMPONENT
    PhongBidirReflDistFunc *brdf;
    PhongBidirTransDistFunc *btdf;
    Texture *texture;

    static ColorRgb splitBsdfEvalTexture(const Texture *texture, const ShadingContext &context);

#ifdef RAYTRACING_ENABLED
    static Vector3Dd
    texturedScattererSample(
        const Vector3Dd * /*in*/,
        const Vector3Dd *normal,
        double x1,
        double x2,
        double *probabilityDensityFunction);

    static void
    texturedScattererEvalPdf(
        const Vector3Dd * /*in*/,
        const Vector3Dd *out,
        const Vector3Dd *normal,
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
        const Vector3Dd * /*in*/,
        const Vector3Dd * /*out*/,
        const Vector3Dd * /*normal*/);
#endif

public:
    explicit PhongBidirScattDistFunc(PhongBidirReflDistFunc *brdf, PhongBidirTransDistFunc *btdf, Texture *texture);
    virtual ~PhongBidirScattDistFunc();

    const PhongBidirReflDistFunc *getBrdf() const;
    const PhongBidirTransDistFunc *getBtdf() const;
    const Texture *getTexture() const;

    static bool bsdfShadingFrame(
        const ShadingContext &context,
        const Vector3Dd *X,
        const Vector3Dd *Y,
        const Vector3Dd *Z);

    ColorRgb splitBsdfScatteredPower(const ShadingContext &context, char flags) const;
    bool splitBsdfIsTextured() const;

#ifdef RAYTRACING_ENABLED
    void indexOfRefraction(RefractionIndex *index) const;

    Vector3Dd
    sample(
        const ShadingContext &context,
        const PhongBidirScattDistFunc *inBsdf,
        const PhongBidirScattDistFunc *outBsdf,
        const Vector3Dd *in,
        int doRussianRoulette,
        char flags,
        double x1,
        double x2,
        double *probabilityDensityFunction) const;

    ColorRgb
    evaluate(
        const ShadingContext &context,
        const PhongBidirScattDistFunc *inBsdf,
        const PhongBidirScattDistFunc *outBsdf,
        const Vector3Dd *in,
        const Vector3Dd *out,
        char flags) const;

    void
    evalProbDensFunc(
        const ShadingContext &context,
        const PhongBidirScattDistFunc *inBsdf,
        const PhongBidirScattDistFunc *outBsdf,
        const Vector3Dd *in,
        const Vector3Dd *out,
        char flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR) const;

    ColorRgb
    bsdfEvalComponents(
        const ShadingContext &context,
        const PhongBidirScattDistFunc *inBsdf,
        const PhongBidirScattDistFunc *outBsdf,
        const Vector3Dd *in,
        const Vector3Dd *out,
        char flags,
        ColorRgb *colArray) const;

#endif

};

inline const PhongBidirReflDistFunc *
PhongBidirScattDistFunc::getBrdf() const {
    return brdf;
}

inline const PhongBidirTransDistFunc *
PhongBidirScattDistFunc::getBtdf() const {
    return btdf;
}

inline const Texture *
PhongBidirScattDistFunc::getTexture() const {
    return texture;
}

#endif
