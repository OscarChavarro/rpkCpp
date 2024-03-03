/**
Emittance Distribution Functions: the self-emitted radiance
distribution of light sources
*/

#ifndef __EDF__
#define __EDF__

#include <cstdio>

#include "material/edf_methods.h"

class EDF {
  public:
    void *data;
    EDF_METHODS *methods;
};

extern EDF *edfCreate(void *data, EDF_METHODS *methods);
extern COLOR edfEmittance(EDF *edf, RayHit *hit, XXDFFLAGS flags);
extern int edfIsTextured(EDF *edf);

extern COLOR
edfEval(
    EDF *edf,
    RayHit *hit,
    Vector3D *out,
    XXDFFLAGS flags,
    double *pdf);

extern Vector3D
edfSample(
    EDF *edf,
    RayHit *hit,
    XXDFFLAGS flags,
    double xi1,
    double xi2,
    COLOR *emitted_radiance,
    double *pdf);

extern int
edfShadingFrame(
    EDF *edf,
    RayHit *hit,
    Vector3D *X,
    Vector3D *Y,
    Vector3D *Z);

#endif
