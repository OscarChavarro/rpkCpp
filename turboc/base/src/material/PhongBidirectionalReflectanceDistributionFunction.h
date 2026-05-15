#ifndef PHNG_BDRCT_RFLCT_DSTRB_FNCTN
#define PHNG_BDRCT_RFLCT_DSTRB_FNCTN

#include "common/linealAlgebra/Vector3D.h"
#include "common/color/ColorRgb.h"

/**
BRDF evaluation functions :
  Vector3D in : incoming ray direction (to patch)
  Vector3D out : reflected ray direction (from patch)
  Vector3D normal : normal vector
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
    ColorRgb evaluate(const Vector3D *in, const Vector3D *out, const Vector3D *normal, char flags) const;

    Vector3D
    sample(
        const Vector3D *in,
        const Vector3D *normal,
        int doRussianRoulette,
        char flags,
        double x1,
        double x2,
        double *probabilityDensityFunction) const;

    void
    evalProbDensFunc(
        const Vector3D *in,
        const Vector3D *out,
        const Vector3D *normal,
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
