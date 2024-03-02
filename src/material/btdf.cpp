/**
Bidirectional Transmittance Distribution Functions
*/

#include "material/btdf.h"

/**
Creates a BTDF instance with given data and methods
*/
BTDF *
btdfCreate(void *data, BTDF_METHODS *methods) {
    BTDF *btdf;

    btdf = (BTDF *)malloc(sizeof(BTDF));
    btdf->data = data;
    btdf->methods = methods;

    return btdf;
}

/**
Returns the transmittance of the BTDF
*/
COLOR
btdfTransmittance(BTDF *btdf, XXDFFLAGS flags) {
    if ( btdf && btdf->methods->Transmittance ) {
        return btdf->methods->Transmittance(btdf->data, flags);
    } else {
        static COLOR refl;
        colorClear(refl);
        return refl;
    }
}

/**
Returns the index of refraction of the BTDF
*/
void
btdfIndexOfRefraction(BTDF *btdf, REFRACTIONINDEX *index) {
    if ( btdf && btdf->methods->IndexOfRefraction ) {
        btdf->methods->IndexOfRefraction(btdf->data, index);
    } else {
        index->nr = 1.0;
        index->ni = 0.0; // Vacuum
    }
}

COLOR
btdfEval(
    BTDF *btdf,
    REFRACTIONINDEX inIndex,
    REFRACTIONINDEX outIndex,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    XXDFFLAGS flags)
{
    if ( btdf && btdf->methods->Eval ) {
        return btdf->methods->Eval(btdf->data, inIndex, outIndex, in, out, normal, flags);
    } else {
        static COLOR refl;
        colorClear(refl);
        return refl;
    }
}

Vector3D
btdfSample(
    BTDF *btdf,
    REFRACTIONINDEX inIndex,
    REFRACTIONINDEX outIndex,
    Vector3D *in,
    Vector3D *normal,
    int doRussianRoulette,
    XXDFFLAGS flags,
    double x1,
    double x2,
    double *pdf)
{
    if ( btdf && btdf->methods->Sample ) {
        return btdf->methods->Sample(btdf->data, inIndex, outIndex, in, normal,
                                     doRussianRoulette, flags, x1, x2, pdf);
    } else {
        Vector3D dummy = {0.0, 0.0, 0.0};
        *pdf = 0;
        return dummy;
    }
}

void
btdfEvalPdf(
    BTDF *btdf,
    REFRACTIONINDEX inIndex,
    REFRACTIONINDEX outIndex,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    XXDFFLAGS flags,
    double *pdf,
    double *pdfRR)
{
    if ( btdf && btdf->methods->EvalPdf ) {
        btdf->methods->EvalPdf(
            btdf->data, inIndex, outIndex, in, out,
            normal, flags, pdf, pdfRR);
    } else {
        *pdf = 0;
    }
}

/**
print the btdf data the to specified file pointer
*/
void
btdfPrint(FILE *out, BTDF *btdf) {
    if ( !btdf ) {
        fprintf(out, "(NULL BTDF)\n");
    } else {
        btdf->methods->Print(out, btdf->data);
    }
}
