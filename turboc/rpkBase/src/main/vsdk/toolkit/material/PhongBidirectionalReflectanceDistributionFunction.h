#ifndef PHNG_BDRCT_RFLCT_DSTRB_FNCTN
#define PHNG_BDRCT_RFLCT_DSTRB_FNCTN

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"

/**
BRDF evaluation functions :
  Vector3Dd in : incoming ray direction (to patch)
  Vector3Dd out : reflected ray direction (from patch)
  Vector3Dd normal : normal vector
  char flags : flags indicating which components must be
    evaluated
*/
class PhongBidirReflDistFunc {
  private:
    ColorRgb Kd;
    ColorRgb Ks;
    double avgKd;
    double avgKs;
    double Ns;

    bool isSpecular() const;

public:
    explicit PhongBidirReflDistFunc(const ColorRgb *Kd, const ColorRgb *Ks, double Ns);
    virtual ~PhongBidirReflDistFunc();

    const ColorRgb &getKd() const;
    const ColorRgb &getKs() const;
    double getNs() const;

    ColorRgb reflectance(char flags) const;
    ColorRgb evaluate(const Vector3Dd *in, const Vector3Dd *out, const Vector3Dd *normal, char flags) const;

    Vector3Dd
    sample(
        const Vector3Dd *in,
        const Vector3Dd *normal,
        int doRussianRoulette,
        char flags,
        double x1,
        double x2,
        double *probabilityDensityFunction) const;

    void
    evalProbDensFunc(
        const Vector3Dd *in,
        const Vector3Dd *out,
        const Vector3Dd *normal,
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

inline double
PhongBidirReflDistFunc::getNs() const {
    return Ns;
}

#endif
