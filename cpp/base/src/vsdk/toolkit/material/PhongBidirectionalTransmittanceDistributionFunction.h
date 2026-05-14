#ifndef PHONG_BIDIRECTIONAL_TRANSMITTANCE_DISTRIBUTION_FUNCTION__
#define PHONG_BIDIRECTIONAL_TRANSMITTANCE_DISTRIBUTION_FUNCTION__

#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "RendererConfiguration.h"
#include "vsdk/toolkit/material/RefractionIndex.h"

/**
Index of refraction data type. Normally when using BSDF's
this should not be needed. In C++ this would of course
be a plain complex number
*/
class PhongBidirectionalTransmittanceDistributionFunction {
  private:
    ColorRgbMutable Kd;
    ColorRgbMutable Ks;
    float avgKd;
    float avgKs;
    float Ns;
    RefractionIndex refractionIndex;

    bool isSpecular() const;

  public:
    explicit PhongBidirectionalTransmittanceDistributionFunction(const ColorRgbMutable *inKd, const ColorRgbMutable *inKs, float inNs, float inNr, float inNi);
    virtual ~PhongBidirectionalTransmittanceDistributionFunction();

    const ColorRgbMutable &getKd() const;
    const ColorRgbMutable &getKs() const;
    float getNs() const;
    const RefractionIndex &getRefractionIndex() const;

    ColorRgbMutable transmittance(char flags) const;

    ColorRgbMutable
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

inline const ColorRgbMutable &
PhongBidirectionalTransmittanceDistributionFunction::getKd() const {
    return Kd;
}

inline const ColorRgbMutable &
PhongBidirectionalTransmittanceDistributionFunction::getKs() const {
    return Ks;
}

inline float
PhongBidirectionalTransmittanceDistributionFunction::getNs() const {
    return Ns;
}

inline const RefractionIndex &
PhongBidirectionalTransmittanceDistributionFunction::getRefractionIndex() const {
    return refractionIndex;
}

#endif
