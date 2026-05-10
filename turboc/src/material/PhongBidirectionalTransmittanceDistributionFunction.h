#ifndef PHNG_BDRCT_TRNSM_DSTRB_FNCTN
#define PHNG_BDRCT_TRNSM_DSTRB_FNCTN

#include "common/linealAlgebra/Vector3D.h"
#include "common/color/ColorRgb.h"
#include "common/RenderOptions.h"
#include "material/RefractionIndex.h"

/**
Index of refraction data type. Normally when using BSDF's
this should not be needed. In C++ this would of course
be a plain complex number
*/
class PhongBidirTransDistFunc {
  private:
    ColorRgb Kd;
    ColorRgb Ks;
    float avgKd;
    float avgKs;
    float Ns;
    RefractionIndex refractionIndex;

    bool isSpecular() const;

  public:
    explicit PhongBidirTransDistFunc(const ColorRgb *inKd, const ColorRgb *inKs, float inNs, float inNr, float inNi);
    virtual ~PhongBidirTransDistFunc();

    const ColorRgb &getKd() const;
    const ColorRgb &getKs() const;
    float getNs() const;
    const RefractionIndex &getRefractionIndex() const;

    ColorRgb transmittance(char flags) const;

    ColorRgb
    evaluate(
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        const Vector3D *in,
        const Vector3D *out,
        const Vector3D *normal,
        char flags) const;

    Vector3D
    sample(
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        const Vector3D *in,
        const Vector3D *normal,
        int doRussianRoulette,
        char flags,
        double x1,
        double x2,
        double *probabilityDensityFunction) const;

    void
    evalProbDensFunc(
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        const Vector3D *in,
        const Vector3D *out,
        const Vector3D *normal,
        char flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR) const;

#ifdef RAYTRACING_ENABLED
    void setIndexOfRefraction(RefractionIndex *index) const;
#endif
};

inline const ColorRgb &
PhongBidirTransDistFunc::getKd() const {
    return Kd;
}

inline const ColorRgb &
PhongBidirTransDistFunc::getKs() const {
    return Ks;
}

inline float
PhongBidirTransDistFunc::getNs() const {
    return Ns;
}

inline const RefractionIndex &
PhongBidirTransDistFunc::getRefractionIndex() const {
    return refractionIndex;
}

#endif
