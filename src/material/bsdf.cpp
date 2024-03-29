/**
Bidirectional Reflectance Distribution Functions
*/

#include "material/bsdf.h"
#include "material/splitbsdf.h"

/**
Creates a BSDF instance with given data and methods
*/
BSDF *
bsdfCreate(SPLIT_BSDF *data) {
    BSDF *bsdf = (BSDF *) malloc(sizeof(BSDF));
    bsdf->data = data;
    return bsdf;
}

/**
Returns the scattered power of the BSDF, depending on the flags
*/
COLOR
bsdfScatteredPower(BSDF *bsdf, RayHit *hit, Vector3D *in, char flags) {
    COLOR reflectionColor;
    colorClear(reflectionColor);
    if ( bsdf != nullptr ) {
        reflectionColor = splitBsdfScatteredPower(bsdf->data, hit, in, flags);
    }
    return reflectionColor;
}

int
bsdfIsTextured(BSDF *bsdf) {
    if ( bsdf != nullptr ) {
        return splitBsdfIsTextured(bsdf->data);
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
bool
bsdfShadingFrame(BSDF *bsdf, RayHit *hit, Vector3D *X, Vector3D *Y, Vector3D *Z) {
    //return bsdf->methods->shadingFrame(bsdf->data, hit, X, Y, Z);
    return false;
}

/**
Returns the index of refraction of the BSDF
*/
void
bsdfIndexOfRefraction(BSDF *bsdf, RefractionIndex *index) {
    if ( bsdf != nullptr ) {
        splitBsdfIndexOfRefraction(bsdf->data, index);
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
bsdfEval(BSDF *bsdf, RayHit *hit, BSDF *inBsdf, BSDF *outBsdf, Vector3D *in, Vector3D *out, BSDF_FLAGS flags) {
    if ( bsdf != nullptr ) {
        return splitBsdfEval(bsdf->data, hit, inBsdf, outBsdf, in, out, flags);
    } else {
        static COLOR reflectionColor;
        colorClear(reflectionColor);
        return reflectionColor;
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
        BSDF_FLAGS flags,
        COLOR *colArray)
{
    // Some caching optimisation could be used here
    COLOR result;
    COLOR empty;
    BSDF_FLAGS thisFlag;

    colorClear(empty);
    colorClear(result);

    for ( int i = 0; i < BSDF_COMPONENTS; i++ ) {
        thisFlag = BSDF_INDEX_TO_COMP(i);

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
        BSDF_FLAGS flags,
        double x_1,
        double x_2,
        double *probabilityDensityFunction)
{
    if ( bsdf != nullptr ) {
        return splitBsdfSample(bsdf->data, hit, inBsdf, outBsdf, in,
                                     doRussianRoulette, flags, x_1, x_2, probabilityDensityFunction);
    } else {
        Vector3D dummy = {0.0, 0.0, 0.0};
        *probabilityDensityFunction = 0;
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
        BSDF_FLAGS flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR)
{
    if ( bsdf != nullptr ) {
        splitBsdfEvalPdf(bsdf->data, hit, inBsdf, outBsdf, in, out,
                               flags, probabilityDensityFunction, probabilityDensityFunctionRR);
    } else {
        *probabilityDensityFunction = 0;
    }
}
