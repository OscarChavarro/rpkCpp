/**
Some general functions regarding edf, brdf, btdf, bsdf
*/

#include "java/lang/Math.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "vsdk/toolkit/material/Xxdf.h"

/**
Calculate the ideal reflected ray direction (independent of the brdf)
*/
Vector3Dd
Xxdf::idealReflectedDirection(const Vector3Dd *in, const Vector3Dd *normal) {
    double tmp = 2 * normal->dotProduct(*in);
    Vector3Dd result;

    result = (*normal).multiply(((double)(tmp)));
    result = (*in).subtract(result);
    result = result.normalized(Numeric::EPSILON);

    return result;
}

/**
Calculate the perfect refracted ray direction.
Sets totalInternalReflection to true or false accordingly.
Cfr. [GLAS1989] An Introduction to Raytracing (Glassner)
*/
Vector3Dd
Xxdf::idealRefractedDirection(
    const Vector3Dd *in,
    const Vector3Dd *normal,
    const RefractionIndex inIndex,
    const RefractionIndex outIndex,
    bool *totalInternalReflection)
{
    // Only real part of n for now
    double refractionIndex = inIndex.getNr() / outIndex.getNr();
    double Ci = -in->dotProduct(*normal);
    double Ct2 = 1 + refractionIndex * refractionIndex * (Ci * Ci - 1);

    if ( Ct2 < 0 ) {
        *totalInternalReflection = true;
        return (idealReflectedDirection(in, normal));
    }
    *totalInternalReflection = false;

    double Ct = Math::sqrt(Ct2);
    double normalScale = refractionIndex * Ci - Ct;

    Vector3Dd result;
    result = (*in).multiply(refractionIndex);
    result = result.add((*normal).multiply(((double)(normalScale))));
    result = result.normalized(Numeric::EPSILON);
    return result;
}
