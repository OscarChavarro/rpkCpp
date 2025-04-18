#ifndef __PHONG_BIDIRECTIONAL_REFLECTANCE_DISTRIBUTION_FUNCTION__
#define __PHONG_BIDIRECTIONAL_REFLECTANCE_DISTRIBUTION_FUNCTION__

#include "common/ColorRgb.h"
#include "common/linealAlgebra/Vector3D.h"

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
    ColorRgb Kd;
    ColorRgb Ks;
    float avgKd;
    float avgKs;
    float Ns;

    bool isSpecular() const;

public:
    explicit PhongBidirectionalReflectanceDistributionFunction(const ColorRgb *Kd, const ColorRgb *Ks, double Ns);
    virtual ~PhongBidirectionalReflectanceDistributionFunction();

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
    evaluateProbabilityDensityFunction(
        const Vector3D *in,
        const Vector3D *out,
        const Vector3D *normal,
        char flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR) const;
};

#endif
