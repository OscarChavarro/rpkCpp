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
    static Vector3Df
    texturedScattererSample(
        const Vector3Df * /*in*/,
        const Vector3Df *normal,
        double x1,
        double x2,
        double *probabilityDensityFunction);

    static void
    texturedScattererEvalPdf(
        const Vector3Df * /*in*/,
        const Vector3Df *out,
        const Vector3Df *normal,
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
        const Vector3Df * /*in*/,
        const Vector3Df * /*out*/,
        const Vector3Df * /*normal*/);
#endif

public:
    explicit PhongBidirScattDistFunc(PhongBidirReflDistFunc *brdf, PhongBidirTransDistFunc *btdf, Texture *texture);
    virtual ~PhongBidirScattDistFunc();

    const PhongBidirReflDistFunc *getBrdf() const;
    const PhongBidirTransDistFunc *getBtdf() const;
    const Texture *getTexture() const;

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
        const PhongBidirScattDistFunc *inBsdf,
        const PhongBidirScattDistFunc *outBsdf,
        const Vector3Df *in,
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
        const Vector3Df *in,
        const Vector3Df *out,
        char flags) const;

    void
    evalProbDensFunc(
        const ShadingContext &context,
        const PhongBidirScattDistFunc *inBsdf,
        const PhongBidirScattDistFunc *outBsdf,
        const Vector3Df *in,
        const Vector3Df *out,
        char flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR) const;

    ColorRgb
    bsdfEvalComponents(
        const ShadingContext &context,
        const PhongBidirScattDistFunc *inBsdf,
        const PhongBidirScattDistFunc *outBsdf,
        const Vector3Df *in,
        const Vector3Df *out,
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
