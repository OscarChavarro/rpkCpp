#ifndef __BRDF_METHODS__
#define __BRDF_METHODS__

#include <cstdio>

#include "common/color.h"
#include "material/xxdf.h"
#include "material/hit.h"

/**
BRDF methods: every kind of BRDF needs to have these functions
implemented
*/
class BRDF_METHODS {
  public:
    // Returns the reflectance
    COLOR (*Reflectance)(void *data, XXDFFLAGS flags);

    // Eval brdf
    COLOR (*Eval)(void *data, Vector3D *in, Vector3D *out, Vector3D *normal, XXDFFLAGS flags);

    Vector3D (*Sample)(void *data, Vector3D *in,
                       Vector3D *normal, int doRussianRoulette,
                       XXDFFLAGS flags, double x_1, double x_2,
                       double *pdf);

    void (*EvalPdf)(void *data, Vector3D *in,
                    Vector3D *out, Vector3D *normal,
                    XXDFFLAGS flags, double *pdf, double *pdfRR);

    // Prints the BRDF data to the specified file
    void (*Print)(FILE *out, void *data);

    // Creates a duplicate of the BRDF data
    void *(*Duplicate)(void *data);

    // Creates a BRDF editor widget (included in the material editor implemented
    // in ui_material.c whenever appropriate). Returns the Widget cast to a
    // void * in order not to have to include all X window header files
    void *(*CreateEditor)(void *parent, void *data);

    // Disposes of the BRDF data
    void (*Destroy)(void *data);
};

#endif
