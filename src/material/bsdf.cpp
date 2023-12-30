/* bsdf.c: Bidirectional Reflectance Distribution Functions */

#include <cstdlib>

#include "material/bsdf.h"
#include "common/linealAlgebra/vectorMacros.h"

/**
Creates a BSDF instance with given data and methods
*/
BSDF *
BsdfCreate(void *data, BSDF_METHODS *methods) {
    BSDF *bsdf;

    /*  bsdf = NEWBSDF();*/
    bsdf = (BSDF *) malloc(sizeof(BSDF));
    bsdf->data = data;
    bsdf->methods = methods;

    return bsdf;
}

/* disposes of the memory occupied by the BSDF instance */
void
BsdfDestroy(BSDF *bsdf) {
    if ( !bsdf ) {
        return;
    }
    bsdf->methods->Destroy(bsdf->data);
    free(bsdf);
}

/* Returns the scattered power of the BSDF, depending on the flags */
COLOR
BsdfScatteredPower(BSDF *bsdf, HITREC *hit, Vector3D *in, BSDFFLAGS flags) {
    COLOR refl;
    COLORCLEAR(refl);
    if ( bsdf && bsdf->methods->ScatteredPower ) {
        refl = bsdf->methods->ScatteredPower(bsdf->data, hit, in, flags);
    }
    return refl;
}

int
BsdfIsTextured(BSDF *bsdf) {
    if ( bsdf && bsdf->methods->IsTextured ) {
        return bsdf->methods->IsTextured(bsdf->data);
    }
    return false;
}

int
BsdfShadingFrame(BSDF *bsdf, HITREC *hit, Vector3D *X, Vector3D *Y, Vector3D *Z) {
    if ( bsdf && bsdf->methods->ShadingFrame ) {
        return bsdf->methods->ShadingFrame(bsdf->data, hit, X, Y, Z);
    }
    return false;
}

void
BsdfIndexOfRefraction(BSDF *bsdf, REFRACTIONINDEX *index) {
    if ( bsdf && bsdf->methods->IndexOfRefraction ) {
        bsdf->methods->IndexOfRefraction(bsdf->data, index);
    } else {
        index->nr = 1.0;
        index->ni = 0.0; /* Vacuum */
    }
}


/* Bsdf evaluations */
COLOR
BsdfEval(BSDF *bsdf, HITREC *hit, BSDF *inBsdf, BSDF *outBsdf, Vector3D *in, Vector3D *out, BSDFFLAGS flags) {
    if ( bsdf && bsdf->methods->Eval ) {
        return bsdf->methods->Eval(bsdf->data, hit, inBsdf, outBsdf, in, out, flags);
    } else {
        static COLOR refl;
        COLORCLEAR(refl);
        return refl;
    }
}

/*
 * BsdfEvalComponents :
 * Evaluates all requested components of the BSDF separately and
 * stores the result in 'colArray'.
 * Total evaluation is returned.
 */
COLOR
BsdfEvalComponents(BSDF *bsdf, HITREC *hit, BSDF *inBsdf,
                                BSDF *outBsdf, Vector3D *in, Vector3D *out,
                                BSDFFLAGS flags,
                                COLOR *colArray) {
    /* Some caching optimisation could be used here */
    COLOR result, empty;
    int i;
    BSDFFLAGS thisFlag;

    COLORCLEAR(empty);
    COLORCLEAR(result);

    for ( i = 0; i < BSDFCOMPONENTS; i++ ) {
        thisFlag = BSDF_INDEXTOCOMP(i);

        if ( flags & thisFlag ) {
            colArray[i] = BsdfEval(bsdf, hit, inBsdf, outBsdf, in, out, thisFlag);
            COLORADD(result, colArray[i], result);
        } else {
            colArray[i] = empty;  /* Set to 0 for safety */
        }
    }

    return result;
}

/* Sampling and pdf evaluation */
Vector3D
BsdfSample(BSDF *bsdf, HITREC *hit,
                    BSDF *inBsdf, BSDF *outBsdf,
                    Vector3D *in,
                    int doRussianRoulette, BSDFFLAGS flags,
                    double x_1, double x_2,
                    double *pdf) {
    if ( bsdf && bsdf->methods->Sample ) {
        return bsdf->methods->Sample(bsdf->data, hit, inBsdf, outBsdf, in,
                                     doRussianRoulette, flags, x_1, x_2, pdf);
    } else {
        Vector3D dummy = {0., 0., 0.};
        *pdf = 0;
        return dummy;
    }
}


void
BsdfEvalPdf(BSDF *bsdf, HITREC *hit,
                 BSDF *inBsdf, BSDF *outBsdf,
                 Vector3D *in, Vector3D *out,
                 BSDFFLAGS flags,
                 double *pdf, double *pdfRR) {
    if ( bsdf && bsdf->methods->EvalPdf ) {
        bsdf->methods->EvalPdf(bsdf->data, hit, inBsdf, outBsdf, in, out,
                               flags, pdf, pdfRR);
    } else {
        *pdf = 0;
    }
}
