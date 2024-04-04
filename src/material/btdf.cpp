/**
Bidirectional Transmittance Distribution Functions
*/

#include "material/btdf.h"
#include "material/phong.h"

/**
Returns the transmittance of the BTDF
*/
ColorRgb
btdfTransmittance(PhongBidirectionalTransmittanceDistributionFunction *btdf, char flags) {
    if ( btdf != nullptr ) {
        return phongTransmittance(btdf, flags);
    } else {
        static ColorRgb reflected;
        reflected.clear();
        return reflected;
    }
}

/**
Returns the index of refraction of the BTDF
*/
void
btdfIndexOfRefraction(PhongBidirectionalTransmittanceDistributionFunction *btdf, RefractionIndex *index) {
    if ( btdf != nullptr ) {
        phongIndexOfRefraction(btdf, index);
    } else {
        index->nr = 1.0;
        index->ni = 0.0; // Vacuum
    }
}

ColorRgb
btdfEval(
    PhongBidirectionalTransmittanceDistributionFunction *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    char flags)
{
    if ( btdf != nullptr ) {
        return phongBtdfEval(btdf, inIndex, outIndex, in, out, normal, flags);
    } else {
        ColorRgb reflectedColor;
        reflectedColor.clear();
        return reflectedColor;
    }
}

Vector3D
btdfSample(
    PhongBidirectionalTransmittanceDistributionFunction *btdf,
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
        return phongBtdfSample(btdf, inIndex, outIndex, in, normal,
                                     doRussianRoulette, flags, x1, x2, probabilityDensityFunction);
    } else {
        Vector3D dummy = {0.0, 0.0, 0.0};
        *probabilityDensityFunction = 0;
        return dummy;
    }
}

void
btdfEvalPdf(
    PhongBidirectionalTransmittanceDistributionFunction *btdf,
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
                btdf, inIndex, outIndex, in, out,
                normal, flags, probabilityDensityFunction, probabilityDensityFunctionRR);
    } else {
        *probabilityDensityFunction = 0;
    }
}
