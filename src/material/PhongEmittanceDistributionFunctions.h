/**
Emittance Distribution Functions: the self-emitted radiance
distribution of light sources
*/

#ifndef __PhongEmittanceDistributionFunctions__
#define __PhongEmittanceDistributionFunctions__

#include <cstdio>

#include "material/edf_methods.h"

/**
If a new implementation of EmittanceDistributionFunctions is needed, this class will be
just an implementation of that interface.
*/
class PhongEmittanceDistributionFunctions {
  public:
    PHONG_EDF *data;
    EDF_METHODS *methods;
};

extern PhongEmittanceDistributionFunctions *edfCreate(PHONG_EDF *data, EDF_METHODS *methods);
extern COLOR edfEmittance(PhongEmittanceDistributionFunctions *edf, RayHit *hit, XXDFFLAGS flags);
extern int edfIsTextured(PhongEmittanceDistributionFunctions *edf);

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
    XXDFFLAGS flags,
    double xi1,
    double xi2,
    COLOR *emitted_radiance,
    double *probabilityDensityFunction);

extern int
edfShadingFrame(
    PhongEmittanceDistributionFunctions *edf,
    RayHit *hit,
    Vector3D *X,
    Vector3D *Y,
    Vector3D *Z);

#endif
