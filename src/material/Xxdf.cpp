/**
Some general functions regarding edf, brdf, btdf, bsdf
*/

#include "material/Xxdf.h"

/**
Phong exponent making the difference between glossy and highly specular reflection/transmission.
Choice is arbitrary for the moment.
*/
const float PHONG_LOWEST_SPECULAR_EXP = 250.0f;

/**
Calculate the ideal reflected ray direction (independent of the brdf)
*/
Vector3D
Xxdf::idealReflectedDirection(const Vector3D *in, const Vector3D *normal) {
    double tmp = 2 * normal->dotProduct(*in);
    Vector3D result;

    result.scaledCopy(static_cast<float>(tmp), *normal);
    result.subtraction(*in, result);
    result.normalize(Numeric::EPSILON_FLOAT);

    return result;
}

/**
Calculate the perfect refracted ray direction.
Sets totalInternalReflection to true or false accordingly.
Cfr. [GLAS1989] An Introduction to Raytracing (Glassner)
*/
Vector3D
Xxdf::idealRefractedDirection(
    const Vector3D *in,
    const Vector3D *normal,
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

    float Ct = java::Math::sqrt(Ct2);
    float normalScale = refractionIndex * Ci - Ct;

    Vector3D result;
    result.scaledCopy(refractionIndex, *in);
    result.sumScaled(result, normalScale, *normal);
    result.normalize(Numeric::EPSILON_FLOAT);
    return result;
}
