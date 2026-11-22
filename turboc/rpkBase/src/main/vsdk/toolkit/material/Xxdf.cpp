/**
Some general functions regarding edf, brdf, btdf, bsdf
*/

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/material/Xxdf.h"

/**
Calculate the ideal reflected ray direction (independent of the brdf)
*/
Vector3Df
Xxdf::idealReflectedDirection(const Vector3Df *in, const Vector3Df *normal) {
    double tmp = 2 * normal->dotProduct(*in);
    Vector3Df result;

    result = (*normal).multiply(((float)(tmp)));
    result = (*in).subtract(result);
    result = result.normalized(Numeric::EPSILON_FLOAT);

    return result;
}

/**
Calculate the perfect refracted ray direction.
Sets totalInternalReflection to true or false accordingly.
Cfr. [GLAS1989] An Introduction to Raytracing (Glassner)
*/
Vector3Df
Xxdf::idealRefractedDirection(
    const Vector3Df *in,
    const Vector3Df *normal,
    const RefractionIndex inIndex,
    const RefractionIndex outIndex,
    bool *totalInternalReflection)
{
    // Only real part of n for now
    float refractionIndex = inIndex.getNr() / outIndex.getNr();
    float Ci = -in->dotProduct(*normal);
    float Ct2 = 1 + refractionIndex * refractionIndex * (Ci * Ci - 1);

    if ( Ct2 < 0 ) {
        *totalInternalReflection = true;
        return (idealReflectedDirection(in, normal));
    }
    *totalInternalReflection = false;

    float Ct = Math::sqrt(Ct2);
    float normalScale = refractionIndex * Ci - Ct;

    Vector3Df result;
    result = (*in).multiply(refractionIndex);
    result = result.add((*normal).multiply(((float)(normalScale))));
    result = result.normalized(Numeric::EPSILON_FLOAT);
    return result;
}
