#ifndef __PHONG_BIDIRECTIONAL_REFLECTANCE_DISTRIBUTION_FUNCTION__
#define __PHONG_BIDIRECTIONAL_REFLECTANCE_DISTRIBUTION_FUNCTION__

#include "common/ColorRgb.h"
#include "common/linealAlgebra/Vector3D.h"

/**
Phong exponent making the difference between glossy and highly specular reflection/transmission.
Choice is arbitrary for the moment.
*/
#define PHONG_LOWEST_SPECULAR_EXP 250
#define PHONG_IS_SPECULAR(p) ((p).Ns >= PHONG_LOWEST_SPECULAR_EXP)

class PhongBidirectionalReflectanceDistributionFunction {
  public:
    ColorRgb Kd;
    ColorRgb Ks;
    float avgKd;
    float avgKs;
    float Ns;

    explicit PhongBidirectionalReflectanceDistributionFunction(const ColorRgb *Kd, const ColorRgb *Ks, double Ns);

    ColorRgb reflectance(char flags) const;
};

extern ColorRgb
phongBrdfEval(
    const PhongBidirectionalReflectanceDistributionFunction *brdf,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    char flags);

extern Vector3D
phongBrdfSample(
    const PhongBidirectionalReflectanceDistributionFunction *brdf,
    const Vector3D *in,
    const Vector3D *normal,
    int doRussianRoulette,
    char flags,
    double x1,
    double x2,
    double *probabilityDensityFunction);

extern void
evaluateProbabilityDensityFunction(
    const PhongBidirectionalReflectanceDistributionFunction *brdf,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR);

#endif
