#ifndef PHNG_BDRCT_RFLCT_DSTRB_FNCTN
#define PHNG_BDRCT_RFLCT_DSTRB_FNCTN

#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"

/**
BRDF evaluation functions :
  Vector3Df in : incoming ray direction (to patch)
  Vector3Df out : reflected ray direction (from patch)
  Vector3Df normal : normal vector
  char flags : flags indicating which components must be
    evaluated
*/
class PhongBidirReflDistFunc {
  private:
    ColorRgb Kd;
    ColorRgb Ks;
    float avgKd;
    float avgKs;
    float Ns;

    bool isSpecular() const;

public:
    explicit PhongBidirReflDistFunc(const ColorRgb *Kd, const ColorRgb *Ks, double Ns);
    virtual ~PhongBidirReflDistFunc();

    const ColorRgb &getKd() const;
    const ColorRgb &getKs() const;
    float getNs() const;

    ColorRgb reflectance(char flags) const;
    ColorRgb evaluate(const Vector3Df *in, const Vector3Df *out, const Vector3Df *normal, char flags) const;

    Vector3Df
    sample(
        const Vector3Df *in,
        const Vector3Df *normal,
        int doRussianRoulette,
        char flags,
        double x1,
        double x2,
        double *probabilityDensityFunction) const;

    void
    evalProbDensFunc(
        const Vector3Df *in,
        const Vector3Df *out,
        const Vector3Df *normal,
        char flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR) const;
};

inline const ColorRgb &
PhongBidirReflDistFunc::getKd() const {
    return Kd;
}

inline const ColorRgb &
PhongBidirReflDistFunc::getKs() const {
    return Ks;
}

inline float
PhongBidirReflDistFunc::getNs() const {
    return Ns;
}

#endif
