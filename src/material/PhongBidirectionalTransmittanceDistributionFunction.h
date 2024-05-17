#ifndef __PHONG_BIDIRECTIONAL_TRANSMITTANCE_DISTRIBUTION_FUNCTION__
#define __PHONG_BIDIRECTIONAL_TRANSMITTANCE_DISTRIBUTION_FUNCTION__

#include "common/RenderOptions.h"
#include "common/ColorRgb.h"
#include "common/linealAlgebra/Vector3D.h"
#include "material/RefractionIndex.h"

/**
Index of refraction data type. Normally when using BSDF's
this should not be needed. In C++ this would of course
be a plain complex number
*/
class PhongBidirectionalTransmittanceDistributionFunction {
  private:
    ColorRgb Kd;
    ColorRgb Ks;
    float avgKd;
    float avgKs;
    float Ns;
    RefractionIndex refractionIndex;

    bool isSpecular() const;

  public:
    explicit PhongBidirectionalTransmittanceDistributionFunction(const ColorRgb *inKd, const ColorRgb *inKs, float inNs, float inNr, float inNi);
    virtual ~PhongBidirectionalTransmittanceDistributionFunction();

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
    evaluateProbabilityDensityFunction(
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

#endif
