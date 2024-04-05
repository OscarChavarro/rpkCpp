/**
Bidirectional Reflectance Distribution Functions
*/

#include "common/error.h"
#include "material/brdf.h"
#include "material/phong.h"

/**
Returns the diffuse reflectance of the BRDF according to the flags
*/
ColorRgb
brdfReflectance(PhongBidirectionalReflectanceDistributionFunction *brdf, char flags) {
    if ( brdf != nullptr ) {
        ColorRgb test = phongReflectance(brdf, flags);
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
    PhongBidirectionalReflectanceDistributionFunction *brdf,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    char flags)
{
    if ( brdf != nullptr ) {
        return phongBrdfEval(brdf, in, out, normal, flags);
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
        PhongBidirectionalReflectanceDistributionFunction *brdf,
        Vector3D *in,
        Vector3D *normal,
        int doRussianRoulette,
        char flags,
        double x_1,
        double x_2,
        double *probabilityDensityFunction)
{
    if ( brdf != nullptr ) {
        return phongBrdfSample(brdf, in, normal,
                                     doRussianRoulette, flags, x_1, x_2, probabilityDensityFunction);
    } else {
        Vector3D dummy = {0.0, 0.0, 0.0};
        *probabilityDensityFunction = 0;
        return dummy;
    }
}

void
brdfEvalPdf(
        PhongBidirectionalReflectanceDistributionFunction *brdf,
        Vector3D *in,
        Vector3D *out,
        Vector3D *normal,
        char flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR)
{
    if ( brdf != nullptr ) {
        phongBrdfEvalPdf(brdf, in, out,
                                   normal, flags, probabilityDensityFunction, probabilityDensityFunctionRR);
    } else {
        *probabilityDensityFunction = 0;
    }
}
