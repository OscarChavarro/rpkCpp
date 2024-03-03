/**
Bidirectional Reflectance Distribution Functions
*/

#include "material/bsdf.h"

/**
Creates a BSDF instance with given data and methods
*/
BSDF *
bsdfCreate(void *data, BSDF_METHODS *methods) {
    BSDF *bsdf = (BSDF *) malloc(sizeof(BSDF));
    bsdf->data = data;
    bsdf->methods = methods;
    return bsdf;
}

/**
Returns the scattered power of the BSDF, depending on the flags
*/
COLOR
bsdfScatteredPower(BSDF *bsdf, RayHit *hit, Vector3D *in, BSDFFLAGS flags) {
    COLOR refl;
    colorClear(refl);
    if ( bsdf && bsdf->methods->ScatteredPower ) {
        refl = bsdf->methods->ScatteredPower(bsdf->data, hit, in, flags);
    }
    return refl;
}

int
bsdfIsTextured(BSDF *bsdf) {
    if ( bsdf && bsdf->methods->IsTextured ) {
        return bsdf->methods->IsTextured(bsdf->data);
    }
    return false;
}

/**
Computes a shading frame at the given hit point. The Z axis of this frame is
the shading normal, The X axis is in the tangent plane on the surface at the
hit point ("brush" direction relevant for anisotropic shaders e.g.). Y
is perpendicular to X and Z. X and Y may be null pointers. In this case,
only the shading normal is returned, avoiding computation of the X and
Y axis if possible).
Note: also edf's can have a routine for computing the shading frame. If a
material has both an edf and a bsdf, the shading frame shall of course
be the same.
This routine returns TRUE if a shading frame could be constructed and FALSE if
not. In the latter case, a default frame needs to be used (not computed by this
routine - hitPointShadingFrame() in material.[ch] constructs such a frame if
needed)
*/
int
bsdfShadingFrame(BSDF *bsdf, RayHit *hit, Vector3D *X, Vector3D *Y, Vector3D *Z) {
    if ( bsdf && bsdf->methods->ShadingFrame ) {
        return bsdf->methods->ShadingFrame(bsdf->data, hit, X, Y, Z);
    }
    return false;
}

/**
Returns the index of refraction of the BSDF
*/
void
bsdfIndexOfRefraction(BSDF *bsdf, RefractionIndex *index) {
    if ( bsdf && bsdf->methods->IndexOfRefraction ) {
        bsdf->methods->IndexOfRefraction(bsdf->data, index);
    } else {
        index->nr = 1.0;
        index->ni = 0.0; /* Vacuum */
    }
}

/**
Bsdf evaluations
All components of the Bsdf

Vector directions :

in : from patch !!
out : from patch
hit->normal : leaving from patch, on the incoming side.
         So in . hit->normal > 0 !!!
*/
COLOR
bsdfEval(BSDF *bsdf, RayHit *hit, BSDF *inBsdf, BSDF *outBsdf, Vector3D *in, Vector3D *out, BSDFFLAGS flags) {
    if ( bsdf && bsdf->methods->Eval ) {
        return bsdf->methods->Eval(bsdf->data, hit, inBsdf, outBsdf, in, out, flags);
    } else {
        static COLOR refl;
        colorClear(refl);
        return refl;
    }
}

/**
bsdfEvalComponents :
Evaluates all requested components of the BSDF separately and
stores the result in 'colArray'.
Total evaluation is returned.
*/
COLOR
bsdfEvalComponents(
    BSDF *bsdf,
    RayHit *hit,
    BSDF *inBsdf,
    BSDF *outBsdf,
    Vector3D *in,
    Vector3D *out,
    BSDFFLAGS flags,
    COLOR *colArray)
{
    // Some caching optimisation could be used here
    COLOR result;
    COLOR empty;
    BSDFFLAGS thisFlag;

    colorClear(empty);
    colorClear(result);

    for ( int i = 0; i < BSDFCOMPONENTS; i++ ) {
        thisFlag = BSDF_INDEXTOCOMP(i);

        if ( flags & thisFlag ) {
            colArray[i] = bsdfEval(bsdf, hit, inBsdf, outBsdf, in, out, thisFlag);
            colorAdd(result, colArray[i], result);
        } else {
            colArray[i] = empty;  /* Set to 0 for safety */
        }
    }

    return result;
}

/**
Sampling and pdf evaluation

Sampling routines, parameters as in evaluation, except that two
random numbers x_1 and x_2 are needed (2D sampling process)
*/
Vector3D
bsdfSample(
    BSDF *bsdf,
    RayHit *hit,
    BSDF *inBsdf,
    BSDF *outBsdf,
    Vector3D *in,
    int doRussianRoulette,
    BSDFFLAGS flags,
    double x_1,
    double x_2,
    double *pdf)
{
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
bsdfEvalPdf(
    BSDF *bsdf,
    RayHit *hit,
    BSDF *inBsdf,
    BSDF *outBsdf,
    Vector3D *in,
    Vector3D *out,
    BSDFFLAGS flags,
    double *pdf,
    double *pdfRR)
{
    if ( bsdf && bsdf->methods->EvalPdf ) {
        bsdf->methods->EvalPdf(bsdf->data, hit, inBsdf, outBsdf, in, out,
                               flags, pdf, pdfRR);
    } else {
        *pdf = 0;
    }
}
