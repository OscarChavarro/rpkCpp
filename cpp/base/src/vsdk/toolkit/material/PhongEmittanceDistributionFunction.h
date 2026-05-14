#ifndef PhongEmittanceDistributionFunctions__
#define PhongEmittanceDistributionFunctions__

#include "vsdk/toolkit/material/PhongBidirectionalTransmittanceDistributionFunction.h"
#include "vsdk/toolkit/material/ShadingContext.h"

/**
Emittance Distribution Functions: the self-emitted radiance
distribution of light sources
*/
class PhongEmittanceDistributionFunction {
  private:
    ColorRgbMutable Kd;
    ColorRgbMutable kd;
    ColorRgbMutable Ks;
    float Ns;

    bool isSpecular() const;

public:
    explicit PhongEmittanceDistributionFunction(const ColorRgbMutable *KdParameter, const ColorRgbMutable *KsParameter, double NsParameter);
    virtual ~PhongEmittanceDistributionFunction();

    const ColorRgbMutable &getKd() const;
    const ColorRgbMutable &getKs() const;
    float getNs() const;

    static bool edfIsTextured();

    static bool
    edfShadingFrame(
        const ShadingContext &context,
        const Vector3D *X,
        const Vector3D *Y,
        const Vector3D *Z);

    ColorRgbMutable phongEmittance(const ShadingContext *context, char flags) const;

    ColorRgbMutable
    phongEdfEval(
        const ShadingContext *context,
        const Vector3D *out,
        char flags,
        double *probabilityDensityFunction) const;

    Vector3D
    phongEdfSample(
        const ShadingContext *context,
        char flags,
        double xi1,
        double xi2,
        ColorRgbMutable *selfEmittedRadiance,
        double *probabilityDensityFunction) const;

};

inline const ColorRgbMutable &
PhongEmittanceDistributionFunction::getKd() const {
    return Kd;
}

inline const ColorRgbMutable &
PhongEmittanceDistributionFunction::getKs() const {
    return Ks;
}

inline float
PhongEmittanceDistributionFunction::getNs() const {
    return Ns;
}

#endif
