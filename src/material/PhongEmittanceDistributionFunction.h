#ifndef __PhongEmittanceDistributionFunctions__
#define __PhongEmittanceDistributionFunctions__

#include "material/RayHit.h"
#include "material/PhongBidirectionalTransmittanceDistributionFunction.h"

/**
Emittance Distribution Functions: the self-emitted radiance
distribution of light sources
*/
class PhongEmittanceDistributionFunction {
  private:
    ColorRgb Kd;
    ColorRgb kd;
    ColorRgb Ks;
    float Ns;

    bool isSpecular() const;

public:
    explicit PhongEmittanceDistributionFunction(const ColorRgb *KdParameter, const ColorRgb *KsParameter, double NsParameter);
    virtual ~PhongEmittanceDistributionFunction();

    static bool edfIsTextured();

    static bool
    edfShadingFrame(
        const RayHit *hit,
        const Vector3D *X,
        const Vector3D *Y,
        const Vector3D *Z);

    ColorRgb phongEmittance(const RayHit * /*hit*/, char flags) const;

    ColorRgb
    phongEdfEval(
        RayHit *hit,
        const Vector3D *out,
        char flags,
        double *probabilityDensityFunction) const;

    Vector3D
    phongEdfSample(
        RayHit *hit,
        char flags,
        double xi1,
        double xi2,
        ColorRgb *selfEmittedRadiance,
        double *probabilityDensityFunction) const ;
};

#endif
