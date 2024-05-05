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
};

extern ColorRgb edfEmittance(const PhongEmittanceDistributionFunction *edf, const RayHit *hit, char flags);
extern bool edfIsTextured(const PhongEmittanceDistributionFunction *edf);

extern ColorRgb
edfEval(
    const PhongEmittanceDistributionFunction *edf,
    RayHit *hit,
    const Vector3D *out,
    char flags,
    double *probabilityDensityFunction);

extern Vector3D
edfSample(
    const PhongEmittanceDistributionFunction *edf,
    RayHit *hit,
    char flags,
    double xi1,
    double xi2,
    ColorRgb *emittedRadiance,
    double *probabilityDensityFunction);

extern bool
edfShadingFrame(
    const PhongEmittanceDistributionFunction *edf,
    const RayHit *hit,
    const Vector3D *X,
    const Vector3D *Y,
    const Vector3D *Z);

#endif
