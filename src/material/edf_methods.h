/* edf_methods.h: EDF methods */

#ifndef _EDF_METHODS_H_
#define _EDF_METHODS_H_

#include <cstdio>

#include "common/linealAlgebra/vectorMacros.h"
#include "common/color.h"
#include "material/xxdf.h"
#include "material/hit.h"

/* EDF methods: every kind of EDF needs to have these functions
 * implemented. */
class EDF_METHODS {
  public:
    /* returns the emittance */
    COLOR (*Emittance)(void *data, RayHit *hit, XXDFFLAGS flags);

    int (*IsTextured)(void *data);

    /* evaluates the edf */
    COLOR (*Eval)(void *data, RayHit *hit, Vector3D *out, XXDFFLAGS flags, double *pdf);

    /* samples the edf */
    Vector3D
    (*Sample)(void *data, RayHit *hit, XXDFFLAGS flags, double xi1, double xi2, COLOR *emitted_radiance, double *pdf);


    /* Computes shading frame at hit point. Returns TRUE if succesful and
     * FALSE if not. X and Y may be null pointers. */
    int (*ShadingFrame)(void *data, RayHit *hit, Vector3D *X, Vector3D *Y, Vector3D *Z);

    /* prints the EDF data to the specified file */
    void (*Print)(FILE *out, void *data);

    /* creates a duplicate of the EDF data */
    void *(*Duplicate)(void *data);

    /* creates a EDF editor widget (included in the material editor implemented
     * in ui_material.c whenever appropriate). Returns the Widget, casted to a
     * void * in order not to have to include all X window header files. */
    void *(*CreateEditor)(void *parent, void *data);

    /* disposes of the EDF data */
    void (*Destroy)(void *data);
};

#endif
