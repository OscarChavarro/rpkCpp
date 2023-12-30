/* btdf.c: Bidirectional Transmittance Distribution Functions */

#include <cstdlib>

#include "material/btdf.h"

#define NEWBTDF() (BTDF *)malloc(sizeof(BTDF))

/**
Creates a BTDF instance with given data and methods
*/
BTDF *BtdfCreate(void *data, BTDF_METHODS *methods) {
    BTDF *btdf;

    btdf = NEWBTDF();
    btdf->data = data;
    btdf->methods = methods;

    return btdf;
}

/* Returns the transmittance of the BTDF */
COLOR BtdfTransmittance(BTDF *btdf, XXDFFLAGS flags) {
    if ( btdf && btdf->methods->Transmittance ) {
        return btdf->methods->Transmittance(btdf->data, flags);
    } else {
        static COLOR refl;
        COLORCLEAR(refl);
        return refl;
    }
}

void BtdfIndexOfRefraction(BTDF *btdf, REFRACTIONINDEX *index) {
    if ( btdf && btdf->methods->IndexOfRefraction ) {
        btdf->methods->IndexOfRefraction(btdf->data, index);
    } else {
        index->nr = 1.0;
        index->ni = 0.0; /* Vacuum */
    }
}

/* Btdf evaluations */

COLOR BtdfEval(BTDF *btdf, REFRACTIONINDEX inIndex, REFRACTIONINDEX outIndex, Vector3D *in, Vector3D *out, Vector3D *normal,
               XXDFFLAGS flags) {
    if ( btdf && btdf->methods->Eval ) {
        return btdf->methods->Eval(btdf->data, inIndex, outIndex, in, out, normal, flags);
    } else {
        static COLOR refl;
        COLORCLEAR(refl);
        return refl;
    }
}

Vector3D BtdfSample(BTDF *btdf, REFRACTIONINDEX inIndex,
                    REFRACTIONINDEX outIndex, Vector3D *in,
                    Vector3D *normal, int doRussianRoulette,
                    XXDFFLAGS flags, double x_1, double x_2,
                    double *pdf) {
    if ( btdf && btdf->methods->Sample ) {
        return btdf->methods->Sample(btdf->data, inIndex, outIndex, in, normal,
                                     doRussianRoulette, flags, x_1, x_2, pdf);
    } else {
        Vector3D dummy = {0., 0., 0.};
        *pdf = 0;
        return dummy;
    }
}

void BtdfEvalPdf(BTDF *btdf, REFRACTIONINDEX inIndex,
                 REFRACTIONINDEX outIndex, Vector3D *in,
                 Vector3D *out, Vector3D *normal,
                 XXDFFLAGS flags, double *pdf, double *pdfRR) {
    if ( btdf && btdf->methods->EvalPdf ) {
        btdf->methods->EvalPdf(btdf->data, inIndex, outIndex, in, out,
                               normal, flags, pdf, pdfRR);
    } else {
        *pdf = 0;
    }
}

/* Print the btdf data the to specified file pointer */
void BtdfPrint(FILE *out, BTDF *btdf) {
    if ( !btdf ) {
        fprintf(out, "(NULL BTDF)\n");
    } else {
        btdf->methods->Print(out, btdf->data);
    }
}
