/* brdf.c: Bidirectional Reflectance Distribution Functions */

#include <cstdlib>

#include "common/linealAlgebra/vectorMacros.h"
#include "common/error.h"
#include "material/brdf.h"

#define NEWBRDF() (BRDF *)malloc(sizeof(BRDF))

/**
Creates a BRDF instance with given data and methods
*/
BRDF *BrdfCreate(void *data, BRDF_METHODS *methods) {
    BRDF *brdf;

    brdf = NEWBRDF();
    brdf->data = data;
    brdf->methods = methods;

    return brdf;
}

/* Returns the diffuse reflectance of the BRDF */
COLOR BrdfReflectance(BRDF *brdf, XXDFFLAGS flags) {
    if ( brdf && brdf->methods->Reflectance ) {
        COLOR test = brdf->methods->Reflectance(brdf->data, flags);
        if ( !std::isfinite(COLORAVERAGE(test))) {
            Fatal(-1, "BrdfReflectance", "Oops - test Rd is not finite!");
        }
        return test;
    } else {
        static COLOR refl;
        colorClear(refl);
        return refl;
    }
}

/* Brdf evaluations */
COLOR BrdfEval(BRDF *brdf, Vector3D *in, Vector3D *out, Vector3D *normal, XXDFFLAGS flags) {
    if ( brdf && brdf->methods->Eval ) {
        return brdf->methods->Eval(brdf->data, in, out, normal, flags);
    } else {
        static COLOR refl;
        colorClear(refl);
        return refl;
    }
}

/* Sampling and pdf evaluation */
Vector3D BrdfSample(BRDF *brdf, Vector3D *in,
                    Vector3D *normal, int doRussianRoulette,
                    XXDFFLAGS flags, double x_1, double x_2,
                    double *pdf) {
    if ( brdf && brdf->methods->Sample ) {
        return brdf->methods->Sample(brdf->data, in, normal,
                                     doRussianRoulette, flags, x_1, x_2, pdf);
    } else {
        Vector3D dummy = {0., 0., 0.};
        *pdf = 0;
        return dummy;
    }
}

void BrdfEvalPdf(BRDF *brdf,
                 Vector3D *in, Vector3D *out, Vector3D *normal,
                 XXDFFLAGS flags,
                 double *pdf, double *pdfRR) {
    if ( brdf && brdf->methods->EvalPdf ) {
        brdf->methods->EvalPdf(brdf->data, in, out,
                               normal, flags, pdf, pdfRR);
    } else {
        *pdf = 0;
    }
}

/* Print the brdf data the to specified file pointer */
void BrdfPrint(FILE *out, BRDF *brdf) {
    if ( !brdf ) {
        fprintf(out, "(NULL BRDF)\n");
    } else {
        brdf->methods->Print(out, brdf->data);
    }
}
