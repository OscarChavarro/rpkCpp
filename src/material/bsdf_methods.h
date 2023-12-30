/* bsdf_methods.h: BSDF methods */

#ifndef _BSDF_METHODS_H_
#define _BSDF_METHODS_H_

#include <cstdio>

#include "common/linealAlgebra/vectorMacros.h"
#include "material/color.h"
#include "material/xxdf.h"
#include "material/hit.h"

/* BSDF methods: every kind of BSDF needs to have these functions
 * implemented. */
class BSDF_METHODS {
  public:
    /* returns the scattered power (diffuse/glossy/specular
     * reflectance and/or transmittance) according to flags */
    COLOR (*ScatteredPower)(void *data, HITREC *hit, Vector3D *in, BSDFFLAGS flags);

    int (*IsTextured)(void *data);

    /* returns the index of refraction */
    void (*IndexOfRefraction)(void *data, REFRACTIONINDEX *index);

    /* void *incomingBsdf should be BSDF *incomingBsdf */
    COLOR (*Eval)(void *data, HITREC *hit, void *inBsdf, void *outBsdf, Vector3D *in, Vector3D *out, BSDFFLAGS flags);

    Vector3D (*Sample)(void *data, HITREC *hit,
                       void *inBsdf, void *outBsdf,
                       Vector3D *in,
                       int doRussianRoulette, BSDFFLAGS flags,
                       double x_1, double x_2,
                       double *pdf);


    void (*EvalPdf)(void *data, HITREC *hit,
                    void *inBsdf, void *outBsdf,
                    Vector3D *in, Vector3D *out,
                    BSDFFLAGS flags,
                    double *pdf, double *pdfRR);

    /* Constructs shading frame at hit point. Returns TRUE if succesful and
     * FALSE if not. X and Y may be null pointers. */
    int (*ShadingFrame)(void *data, HITREC *hit, Vector3D *X, Vector3D *Y, Vector3D *Z);

    /* prints the BSDF data to the specified file */
    void (*Print)(FILE *out, void *data);

    /* creates a duplicate of the BSDF data */
    void *(*Duplicate)(void *data);

    /* creates a BSDF editor widget (included in the material editor implemented
     * in ui_material.c whenever appropriate). Returns the Widget, casted to a
     * void * in order not to have to include all X window header files. */
    void *(*CreateEditor)(void *parent, void *data);

    /* disposes of the BSDF data */
    void (*Destroy)(void *data);
};

#endif
