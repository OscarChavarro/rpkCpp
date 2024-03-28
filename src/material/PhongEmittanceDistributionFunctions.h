#ifndef __PhongEmittanceDistributionFunctions__
#define __PhongEmittanceDistributionFunctions__

#include <cstdio>

#include "scene/phong.h"

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
    PHONG_EDF *data;
};

extern PhongEmittanceDistributionFunctions *edfCreate(PHONG_EDF *data);
extern COLOR edfEmittance(PhongEmittanceDistributionFunctions *edf, RayHit *hit, char flags);
extern bool edfIsTextured(PhongEmittanceDistributionFunctions *edf);

extern COLOR
edfEval(
    PhongEmittanceDistributionFunctions *edf,
    RayHit *hit,
    Vector3D *out,
    XXDFFLAGS flags,
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
