/**
Some general functions regarding edf, brdf, btdf, bsdf
*/

#include "material/xxdf.h"

/**
Compute an approximate Geometric IOR from a complex IOR (cfr. Gr.Gems II, p289)
*/
float
complexToGeometricRefractionIndex(RefractionIndex nc) {
    float sqrtF;
    float f1;
    float f2;

    f1 = (nc.nr - 1.0f);
    f1 = f1 * f1 + nc.ni * nc.ni;

    f2 = (nc.nr + 1.0f);
    f2 = f2 * f2 + nc.ni * nc.ni;

    sqrtF = std::sqrt(f1 / f2);

    return (1.0f + sqrtF) / (1.0f - sqrtF);
}

/**
Calculate the ideal reflected ray direction (independent of the brdf)
*/
Vector3D
idealReflectedDirection(const Vector3D *in, const Vector3D *normal) {
    double tmp;
    Vector3D result;

    tmp = 2 * vectorDotProduct(*normal, *in);
    vectorScale((float)tmp, *normal, result);
    vectorSubtract(*in, result, result);
    vectorNormalize(result);

    return result;
}

/**
Calculate the perfect refracted ray direction.
Sets totalInternalReflection to true or false accordingly.
Cfr. An Introduction to Global_Raytracer_activeRaytracer (Glassner)
*/
Vector3D
idealRefractedDirection(
    const Vector3D *in,
    const Vector3D *normal,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    int *totalInternalReflection)
{
    double Ci;
    double Ct;
    double Ct2;
    double refractionIndex;
    double normalScale;
    Vector3D result;

    // Only real part of n for now
    refractionIndex = inIndex.nr / outIndex.nr;

    Ci = -vectorDotProduct(*in, *normal);
    Ct2 = 1 + refractionIndex * refractionIndex * (Ci * Ci - 1);

    if ( Ct2 < 0 ) {
        *totalInternalReflection = true;
        return (idealReflectedDirection(in, normal));
    }
    *totalInternalReflection = false;

    Ct = std::sqrt(Ct2);

    normalScale = refractionIndex * Ci - Ct;

    vectorScale((float)refractionIndex, *in, result);
    vectorSumScaled(result, normalScale, *normal, result);

    // Normalise
    vectorNormalize(result);

    return result;
}
