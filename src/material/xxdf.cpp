/**
Some general functions regarding edf, brdf, btdf, bsdf
*/

#include "material/xxdf.h"

/**
Compute an approximate geometric IOR from a complex IOR (cfr. Gr.Gems II, p289)
*/
float
RefractionIndex::complexToGeometricRefractionIndex() const {
    float f1 = (nr - 1.0f);
    f1 = f1 * f1 + ni * ni;

    float f2 = (nr + 1.0f);
    f2 = f2 * f2 + ni * ni;

    float sqrtF = std::sqrt(f1 / f2);

    return (1.0f + sqrtF) / (1.0f - sqrtF);
}

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
    float refractionIndex = inIndex.nr / outIndex.nr;
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
