/**
Bidirectional Transmittance Distribution Functions
*/

#include "material/btdf.h"
#include "scene/phong.h"

/**
Creates a BTDF instance with given data and methods
*/
BTDF *
btdfCreate(PHONG_BTDF *data) {
    BTDF *btdf = new BTDF();
    btdf->data = data;
    return btdf;
}

/**
Returns the transmittance of the BTDF
*/
COLOR
btdfTransmittance(BTDF *btdf, char flags) {
    if ( btdf != nullptr ) {
        return phongTransmittance(btdf->data, flags);
    } else {
        static COLOR reflected;
        colorClear(reflected);
        return reflected;
    }
}

/**
Returns the index of refraction of the BTDF
*/
void
btdfIndexOfRefraction(BTDF *btdf, RefractionIndex *index) {
    if ( btdf != nullptr ) {
        phongIndexOfRefraction(btdf->data, index);
    } else {
        index->nr = 1.0;
        index->ni = 0.0; // Vacuum
    }
}

COLOR
btdfEval(
    BTDF *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    char flags)
{
    if ( btdf != nullptr ) {
        return phongBtdfEval(btdf->data, inIndex, outIndex, in, out, normal, flags);
    } else {
        COLOR reflectedColor;
        colorClear(reflectedColor);
        return reflectedColor;
    }
}

Vector3D
btdfSample(
    BTDF *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    Vector3D *in,
    Vector3D *normal,
    int doRussianRoulette,
    char flags,
    double x1,
    double x2,
    double *probabilityDensityFunction)
{
    if ( btdf != nullptr ) {
        return phongBtdfSample(btdf->data, inIndex, outIndex, in, normal,
                                     doRussianRoulette, flags, x1, x2, probabilityDensityFunction);
    } else {
        Vector3D dummy = {0.0, 0.0, 0.0};
        *probabilityDensityFunction = 0;
        return dummy;
    }
}

void
btdfEvalPdf(
    BTDF *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR)
{
    if ( btdf != nullptr ) {
        phongBtdfEvalPdf(
                btdf->data, inIndex, outIndex, in, out,
                normal, flags, probabilityDensityFunction, probabilityDensityFunctionRR);
    } else {
        *probabilityDensityFunction = 0;
    }
}
