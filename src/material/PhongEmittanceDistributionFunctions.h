#ifndef __PhongEmittanceDistributionFunctions__
#define __PhongEmittanceDistributionFunctions__

#include <cstdio>

#include "material/hit.h"
#include "material/phong.h"

/**
Emittance Distribution Functions: the self-emitted radiance
distribution of light sources
*/

/**
If a new implementation of EmittanceDistributionFunctions is needed, this class will be
just an implementation of that interface.
*/
class PhongEmittanceDistributionFunctions {
  public:
    COLOR Kd;
    COLOR kd;
    COLOR Ks;
    float Ns;

    PhongEmittanceDistributionFunctions(COLOR *KdParameter, COLOR *KsParameter, double NsParameter);
};

extern COLOR edfEmittance(PhongEmittanceDistributionFunctions *edf, RayHit *hit, char flags);
extern bool edfIsTextured(PhongEmittanceDistributionFunctions *edf);

extern COLOR
edfEval(
    PhongEmittanceDistributionFunctions *edf,
    RayHit *hit,
    Vector3D *out,
    char flags,
    double *probabilityDensityFunction);

extern Vector3D
edfSample(
    PhongEmittanceDistributionFunctions *edf,
    RayHit *hit,
    char flags,
    double xi1,
    double xi2,
    COLOR *emittedRadiance,
    double *probabilityDensityFunction);

extern bool
edfShadingFrame(
    PhongEmittanceDistributionFunctions *edf,
    RayHit *hit,
    Vector3D *X,
    Vector3D *Y,
    Vector3D *Z);

#endif
