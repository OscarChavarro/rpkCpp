#ifndef __PhongEmittanceDistributionFunctions__
#define __PhongEmittanceDistributionFunctions__

#include <cstdio>

#include "material/RayHit.h"
#include "material/PhongBidirectionalTransmittanceDistributionFunction.h"

/**
Emittance Distribution Functions: the self-emitted radiance
distribution of light sources
*/

/**
If a new implementation of EmittanceDistributionFunctions is needed, this class will be
just an implementation of that interface.
*/
class PhongEmittanceDistributionFunction {
  public:
    ColorRgb Kd;
    ColorRgb kd;
    ColorRgb Ks;
    float Ns;

    PhongEmittanceDistributionFunction(const ColorRgb *KdParameter, const ColorRgb *KsParameter, double NsParameter);

    ColorRgb phongEmittance(const RayHit * /*hit*/, char flags) const;
    static bool edfIsTextured();

    static bool
    edfShadingFrame(
        const RayHit *hit,
        const Vector3D *X,
        const Vector3D *Y,
        const Vector3D *Z);

    ColorRgb
    phongEdfEval(
        RayHit *hit,
        const Vector3D *out,
        char flags,
        double *probabilityDensityFunction) const;
};

extern Vector3D
edfSample(
    const PhongEmittanceDistributionFunction *edf,
    RayHit *hit,
    char flags,
    double xi1,
    double xi2,
    ColorRgb *emittedRadiance,
    double *probabilityDensityFunction);

#endif
