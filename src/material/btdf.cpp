/**
Bidirectional Transmittance Distribution Functions
*/

#include "material/btdf.h"
#include "material/PhongBidirectionalTransmittanceDistributionFunction.h"

/**
Returns the transmittance of the BTDF
*/
ColorRgb
btdfTransmittance(const PhongBidirectionalTransmittanceDistributionFunction *btdf, char flags) {
    if ( btdf != nullptr ) {
        return btdf->transmittance(flags);
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
btdfIndexOfRefraction(const PhongBidirectionalTransmittanceDistributionFunction *btdf, RefractionIndex *index) {
    if ( btdf != nullptr ) {
        btdf->indexOfRefraction(index);
    } else {
        index->nr = 1.0;
        index->ni = 0.0; // Vacuum
    }
}

ColorRgb
btdfEval(
    const PhongBidirectionalTransmittanceDistributionFunction *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    char flags)
{
    if ( btdf != nullptr ) {
        return btdf->evaluate(inIndex, outIndex, in, out, normal, flags);
    } else {
        ColorRgb reflectedColor;
        reflectedColor.clear();
        return reflectedColor;
    }
}

Vector3D
btdfSample(
    const PhongBidirectionalTransmittanceDistributionFunction *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    const Vector3D *in,
    const Vector3D *normal,
    int doRussianRoulette,
    char flags,
    double x1,
    double x2,
    double *probabilityDensityFunction)
{
    if ( btdf != nullptr ) {
        return btdf->sample(
                inIndex, outIndex, in, normal, doRussianRoulette, flags, x1, x2, probabilityDensityFunction);
    } else {
        Vector3D dummy = {0.0, 0.0, 0.0};
        *probabilityDensityFunction = 0;
        return dummy;
    }
}

void
btdfEvalPdf(
    const PhongBidirectionalTransmittanceDistributionFunction *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    const char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR)
{
    if ( btdf != nullptr ) {
        btdf->evaluateProbabilityDensityFunction(
                inIndex,
                outIndex,
                in,
                out,
                normal,
                flags,
                probabilityDensityFunction,
                probabilityDensityFunctionRR);
    } else {
        *probabilityDensityFunction = 0;
    }
}
