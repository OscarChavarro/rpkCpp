#ifndef PHNG_BDRCT_TRNSM_DSTRB_FNCTN
#define PHNG_BDRCT_TRNSM_DSTRB_FNCTN

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/material/RefractionIndex.h"

/**
Index of refraction data type. Normally when using BSDF's
this should not be needed. In C++ this would of course
be a plain complex number
*/
class PhongBidirTransDistFunc {
  private:
    ColorRgb Kd;
    ColorRgb Ks;
    double avgKd;
    double avgKs;
    double Ns;
    RefractionIndex refractionIndex;

    bool isSpecular() const;

  public:
    explicit PhongBidirTransDistFunc(const ColorRgb *inKd, const ColorRgb *inKs, double inNs, double inNr, double inNi);
    virtual ~PhongBidirTransDistFunc();

    const ColorRgb &getKd() const;
    const ColorRgb &getKs() const;
    double getNs() const;
    const RefractionIndex &getRefractionIndex() const;

    ColorRgb transmittance(char flags) const;

    ColorRgb
    evaluate(
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        const Vector3Dd *in,
        const Vector3Dd *out,
        const Vector3Dd *normal,
        char flags) const;

    Vector3Dd
    sample(
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        const Vector3Dd *in,
        const Vector3Dd *normal,
        int doRussianRoulette,
        char flags,
        double x1,
        double x2,
        double *probabilityDensityFunction) const;

    void
    evalProbDensFunc(
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        const Vector3Dd *in,
        const Vector3Dd *out,
        const Vector3Dd *normal,
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

inline double
PhongBidirTransDistFunc::getNs() const {
    return Ns;
}

inline const RefractionIndex &
PhongBidirTransDistFunc::getRefractionIndex() const {
    return refractionIndex;
}

#endif
