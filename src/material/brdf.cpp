/**
Bidirectional Reflectance Distribution Functions
*/

#include "common/error.h"
#include "material/brdf.h"
#include "material/PhongBidirectionalTransmittanceDistributionFunction.h"

/**
Returns the diffuse reflectance of the BRDF according to the flags
*/
ColorRgb
brdfReflectance(const PhongBidirectionalReflectanceDistributionFunction *brdf, const char flags) {
    if ( brdf != nullptr ) {
        ColorRgb test = brdf->reflectance(flags);
        if ( !std::isfinite(test.average()) ) {
            logFatal(-1, "brdfReflectance", "Oops - test Rd is not finite!");
        }
        return test;
    } else {
        ColorRgb reflectedColor;
        reflectedColor.clear();
        return reflectedColor;
    }
}

/**
Brdf evaluations
*/
ColorRgb
brdfEval(
    const PhongBidirectionalReflectanceDistributionFunction *brdf,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    const char flags)
{
    if ( brdf != nullptr ) {
        return brdf->evaluate(in, out, normal, flags);
    } else {
        ColorRgb reflectedColor;
        reflectedColor.clear();
        return reflectedColor;
    }
}

/**
Sampling and Probability Density Function evaluation
*/
Vector3D
brdfSample(
    const PhongBidirectionalReflectanceDistributionFunction *brdf,
    const Vector3D *in,
    const Vector3D *normal,
    const int doRussianRoulette,
    const char flags,
    double x1,
    double x2,
    double *probabilityDensityFunction)
{
    if ( brdf != nullptr ) {
        return brdf->sample(in, normal, doRussianRoulette, flags, x1, x2, probabilityDensityFunction);
    } else {
        Vector3D dummy = {0.0, 0.0, 0.0};
        *probabilityDensityFunction = 0;
        return dummy;
    }
}

void
brdfEvalPdf(
    const PhongBidirectionalReflectanceDistributionFunction *brdf,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    const char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR)
{
    if ( brdf != nullptr ) {
        brdf->evaluateProbabilityDensityFunction(
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
