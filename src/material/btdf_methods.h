#ifndef __BTDF_METHODS__
#define __BTDF_METHODS__

#include "material/color.h"
#include "material/xxdf.h"

/**
BTDF methods: every kind of BTDF needs to have these functions
implemented
*/
class BTDF_METHODS {
  public:
    // Returns the transmittance
    COLOR (*Transmittance)(void *data, XXDFFLAGS flags);

    // Returns the index of refraction
    void (*IndexOfRefraction)(void *data, REFRACTIONINDEX *index);

    COLOR
    (*Eval)(void *data, REFRACTIONINDEX inIndex, REFRACTIONINDEX outIndex, Vector3D *in, Vector3D *out, Vector3D *normal,
            XXDFFLAGS flags);

    Vector3D (*Sample)(void *data, REFRACTIONINDEX inIndex,
                       REFRACTIONINDEX outIndex, Vector3D *in,
                       Vector3D *normal, int doRussianRoulette,
                       XXDFFLAGS flags, double x_1, double x_2,
                       double *pdf);

    void (*EvalPdf)(void *data, REFRACTIONINDEX inIndex,
                    REFRACTIONINDEX outIndex, Vector3D *in,
                    Vector3D *out, Vector3D *normal,
                    XXDFFLAGS flags, double *pdf, double *pdfRR);

    // Prints the BTDF data to the specified file
    void (*Print)(FILE *out, void *data);
};

#endif
