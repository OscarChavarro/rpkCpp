/**
Some general functions regarding edf, brdf, btdf, bsdf
*/

#include "material/xxdf.h"

/**
Calculate the ideal reflected ray direction (independent of the brdf)
*/
Vector3D
idealReflectedDirection(const Vector3D *in, const Vector3D *normal) {
    double tmp = 2 * vectorDotProduct(*normal, *in);
    Vector3D result;

    vectorScale((float)tmp, *normal, result);
    vectorSubtract(*in, result, result);
    vectorNormalize(result);

    return result;
}

/**
Calculate the perfect refracted ray direction.
Sets totalInternalReflection to true or false accordingly.
Cfr. [GLAS1989] An Introduction to Raytracing (Glassner)
*/
Vector3D
idealRefractedDirection(
    const Vector3D *in,
    const Vector3D *normal,
    const RefractionIndex inIndex,
    const RefractionIndex outIndex,
    bool *totalInternalReflection)
{
    // Only real part of n for now
    float refractionIndex = inIndex.getNr() / outIndex.getNr();
    float Ci = -vectorDotProduct(*in, *normal);
    float Ct2 = 1 + refractionIndex * refractionIndex * (Ci * Ci - 1);

    if ( Ct2 < 0 ) {
        *totalInternalReflection = true;
        return (idealReflectedDirection(in, normal));
    }
    *totalInternalReflection = false;

    float Ct = std::sqrt(Ct2);
    float normalScale = refractionIndex * Ci - Ct;

    Vector3D result;
    vectorScale(refractionIndex, *in, result);
    vectorSumScaled(result, normalScale, *normal, result);
    vectorNormalize(result);
    return result;
}
