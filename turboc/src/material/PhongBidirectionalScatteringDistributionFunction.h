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

#include "common/color/ColorRgb.h"
#include "common/RenderOptions.h"
#include "material/PhongBidirectionalReflectanceDistributionFunction.h"
#include "material/PhongBidirectionalTransmittanceDistributionFunction.h"
#include "material/ShadingContext.h"
#include "skin/RayHit.h"
#include "material/Texture.h"
#include "material/Xxdf.h"

#ifdef RAYTRACING_ENABLED
    #include "material/SplitBSDFSamplingMode.h"
#endif

class PhongBidirScattDistFunc {
  private:
    #define TEXTURED_COMPONENT BRDF_DIFFUSE_COMPONENT
    PhongBidirReflDistFunc *brdf;
    PhongBidirTransDistFunc *btdf;
    Texture *texture;

    static ColorRgb splitBsdfEvalTexture(const Texture *texture, const ShadingContext &context);
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
        const ShadingContext &context,
        char flags,
        double *texture,
        double *reflection,
        double *transmission,
        char *brdfFlags,
        char *btdfFlags) const;

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
    explicit PhongBidirScattDistFunc(PhongBidirReflDistFunc *brdf, PhongBidirTransDistFunc *btdf, Texture *texture);
    virtual ~PhongBidirScattDistFunc();

    const PhongBidirReflDistFunc *getBrdf() const;
    const PhongBidirTransDistFunc *getBtdf() const;
    const Texture *getTexture() const;

    static bool bsdfShadingFrame(
        const ShadingContext &context,
        const Vector3D *X,
        const Vector3D *Y,
        const Vector3D *Z);

    static bool bsdfShadingFrame(
        const RayHit *hit,
        const Vector3D *X,
        const Vector3D *Y,
        const Vector3D *Z);

    ColorRgb splitBsdfScatteredPower(const ShadingContext &context, char flags) const;
    ColorRgb splitBsdfScatteredPower(RayHit *hit, char flags) const;
    bool splitBsdfIsTextured() const;

#ifdef RAYTRACING_ENABLED
    void indexOfRefraction(RefractionIndex *index) const;

    Vector3D
    sample(
        const ShadingContext &context,
        const PhongBidirScattDistFunc *inBsdf,
        const PhongBidirScattDistFunc *outBsdf,
        const Vector3D *in,
        int doRussianRoulette,
        char flags,
        double x1,
        double x2,
        double *probabilityDensityFunction) const;

    Vector3D
    sample(
        RayHit *hit,
        const PhongBidirScattDistFunc *inBsdf,
        const PhongBidirScattDistFunc *outBsdf,
        const Vector3D *in,
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
        const Vector3D *in,
        const Vector3D *out,
        char flags) const;

    ColorRgb
    evaluate(
        RayHit *hit,
        const PhongBidirScattDistFunc *inBsdf,
        const PhongBidirScattDistFunc *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        char flags) const;

    void
    evalProbDensFunc(
        const ShadingContext &context,
        const PhongBidirScattDistFunc *inBsdf,
        const PhongBidirScattDistFunc *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        char flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR) const;

    void
    evalProbDensFunc(
        RayHit *hit,
        const PhongBidirScattDistFunc *inBsdf,
        const PhongBidirScattDistFunc *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        char flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR) const;

    ColorRgb
    bsdfEvalComponents(
        const ShadingContext &context,
        const PhongBidirScattDistFunc *inBsdf,
        const PhongBidirScattDistFunc *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        char flags,
        ColorRgb *colArray) const;

    ColorRgb
    bsdfEvalComponents(
        RayHit *hit,
        const PhongBidirScattDistFunc *inBsdf,
        const PhongBidirScattDistFunc *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
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
