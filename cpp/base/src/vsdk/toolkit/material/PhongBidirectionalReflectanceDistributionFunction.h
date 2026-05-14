#ifndef PHONG_BIDIRECTIONAL_REFLECTANCE_DISTRIBUTION_FUNCTION__
#define PHONG_BIDIRECTIONAL_REFLECTANCE_DISTRIBUTION_FUNCTION__

#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"

/**
BRDF evaluation functions :
  Vector3D in : incoming ray direction (to patch)
  Vector3D out : reflected ray direction (from patch)
  Vector3D normal : normal vector
  char flags : flags indicating which components must be
    evaluated
*/
class PhongBidirectionalReflectanceDistributionFunction {
  private:
    ColorRgbMutable Kd;
    ColorRgbMutable Ks;
    float avgKd;
    float avgKs;
    float Ns;

    bool isSpecular() const;

public:
    explicit PhongBidirectionalReflectanceDistributionFunction(const ColorRgbMutable *Kd, const ColorRgbMutable *Ks, double Ns);
    virtual ~PhongBidirectionalReflectanceDistributionFunction();

    const ColorRgbMutable &getKd() const;
    const ColorRgbMutable &getKs() const;
    float getNs() const;

    ColorRgbMutable reflectance(char flags) const;
    ColorRgbMutable evaluate(const Vector3D *in, const Vector3D *out, const Vector3D *normal, char flags) const;

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
    evaluateProbabilityDensityFunction(
        const Vector3D *in,
        const Vector3D *out,
        const Vector3D *normal,
        char flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR) const;
};

inline const ColorRgbMutable &
PhongBidirectionalReflectanceDistributionFunction::getKd() const {
    return Kd;
}

inline const ColorRgbMutable &
PhongBidirectionalReflectanceDistributionFunction::getKs() const {
    return Ks;
}

inline float
PhongBidirectionalReflectanceDistributionFunction::getNs() const {
    return Ns;
}

#endif
