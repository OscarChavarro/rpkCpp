/* edf.c: Emittance Distribution Functions */

#include <cstdlib>

#include "material/edf.h"
#include "common/error.h"

/* Creates a EDF instance with given data and methods. A pointer
 * to the created EDF object is returned. */
EDF *EdfCreate(void *data, EDF_METHODS *methods) {
    EDF *edf = (EDF *)malloc(sizeof(EDF));
    edf->data = data;
    edf->methods = methods;

    return edf;
}

/* Creates and returns a duplicate of the given EDF */
EDF *EdfDuplicate(EDF *oedf) {
    if ( !oedf ) {
        return oedf;
    }

    EDF *edf = (EDF *)malloc(sizeof(EDF));
    edf->data = oedf->methods->Duplicate(oedf->data);
    edf->methods = oedf->methods;

    return edf;
}

/* Creates an editor widget for the EDF, returns the Widget casted
 * to a void * in order not to have to include all X window system files. */
void *EdfCreateEditor(void *parent, EDF *edf) {
    if ( !edf ) {
        Fatal(-1, "EdfCreateEditor", "NULL edf pointer passed.");
    }
    return edf->methods->CreateEditor(parent, edf->data);
}

/* disposes of the memory occupied by the EDF instance */
void EdfDestroy(EDF *edf) {
    if ( !edf ) {
        return;
    }
    edf->methods->Destroy(edf->data);
    free(edf);
}

/* Returns the emittance of the EDF */
COLOR EdfEmittance(EDF *edf, HITREC *hit, XXDFFLAGS flags) {
    if ( edf && edf->methods->Emittance ) {
        return edf->methods->Emittance(edf->data, hit, flags);
    } else {
        static COLOR emit;
        COLORCLEAR(emit);
        return emit;
    }
}

int EdfIsTextured(EDF *edf) {
    if ( edf && edf->methods->IsTextured ) {
        return edf->methods->IsTextured(edf->data);
    }
    return false;
}

/* returns diffusely emitted radiance [W/m^2 sr] */
COLOR EdfDiffuseRadiance(EDF *edf, HITREC *hit) {
    COLOR rad = EdfDiffuseEmittance(edf, hit);
    COLORSCALE((1. / M_PI), rad, rad);
    return rad;
}

/* Evaluates the edf */
COLOR EdfEval(EDF *edf, HITREC *hit, Vector3D *out, XXDFFLAGS flags, double *pdf) {
    if ( edf && edf->methods->Eval ) {
        return edf->methods->Eval(edf->data, hit, out, flags, pdf);
    } else {
        static COLOR val;
        COLORCLEAR(val);
        if ( pdf ) {
            *pdf = 0.;
        }
        return val;
    }
}

Vector3D EdfSample(EDF *edf, HITREC *hit, XXDFFLAGS flags,
                   double xi1, double xi2,
                   COLOR *emitted_radiance, double *pdf) {
    if ( edf && edf->methods->Sample ) {
        return edf->methods->Sample(edf->data, hit, flags, xi1, xi2, emitted_radiance, pdf);
    } else {
        Vector3D v = {0., 0., 0.};
        Fatal(-1, "EdfSample", "Can't sample EDF");
        return v;
    }
}


int EdfShadingFrame(EDF *edf, HITREC *hit, Vector3D *X, Vector3D *Y, Vector3D *Z) {
    if ( edf && edf->methods->ShadingFrame ) {
        return edf->methods->ShadingFrame(edf->data, hit, X, Y, Z);
    }
    return false;
}

