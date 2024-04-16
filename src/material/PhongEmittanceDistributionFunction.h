#ifndef __PhongEmittanceDistributionFunctions__
#define __PhongEmittanceDistributionFunctions__

#include <cstdio>

#include "material/RayHit.h"
#include "material/phong.h"

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

    PhongEmittanceDistributionFunction(ColorRgb *KdParameter, ColorRgb *KsParameter, double NsParameter);
};

extern ColorRgb edfEmittance(PhongEmittanceDistributionFunction *edf, RayHit *hit, char flags);
extern bool edfIsTextured(PhongEmittanceDistributionFunction *edf);

extern ColorRgb
edfEval(
    PhongEmittanceDistributionFunction *edf,
    RayHit *hit,
    Vector3D *out,
    char flags,
    double *probabilityDensityFunction);

extern Vector3D
edfSample(
    PhongEmittanceDistributionFunction *edf,
    RayHit *hit,
    char flags,
    double xi1,
    double xi2,
    ColorRgb *emittedRadiance,
    double *probabilityDensityFunction);

extern bool
edfShadingFrame(
    PhongEmittanceDistributionFunction *edf,
    RayHit *hit,
    Vector3D *X,
    Vector3D *Y,
    Vector3D *Z);

#endif
