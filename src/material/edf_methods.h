#ifndef __EDF_METHODS__
#define __EDF_METHODS__

#include "common/color.h"
#include "material/xxdf.h"
#include "material/hit.h"

/**
EDF methods: every kind of EDF needs to have these functions
implemented
*/
class EDF_METHODS {
  public:
    // Returns the emittance
    COLOR (*Emittance)(void *data, RayHit *hit, XXDFFLAGS flags);
    int (*IsTextured)(void *data);

    // Evaluates the edf
    COLOR (*Eval)(void *data, RayHit *hit, Vector3D *out, XXDFFLAGS flags, double *pdf);

    // Samples the edf
    Vector3D
    (*Sample)(void *data, RayHit *hit, XXDFFLAGS flags, double xi1, double xi2, COLOR *emitted_radiance, double *pdf);


    // Computes shading frame at hit point. Returns TRUE if successful and
    // FALSE if not. X and Y may be null pointers
    int (*ShadingFrame)(void *data, RayHit *hit, Vector3D *X, Vector3D *Y, Vector3D *Z);

    // Prints the EDF data to the specified file
    void (*Print)(FILE *out, void *data);
};

#endif
