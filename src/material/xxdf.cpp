/* xxdf.c : some general functions regarding edf,brdf,btdf,bsdf */

#include "common/error.h"
#include "common/linealAlgebra/vectorMacros.h"
#include "material/xxdf.h"

/* Compute an approximate Geometric IOR from a complex IOR (cfr. Gr.Gems II, p289) */

float ComplexToGeometricRefractionIndex(REFRACTIONINDEX nc) {
    float sqrtF, f1, f2;

    f1 = (nc.nr - 1.0);
    f1 = f1 * f1 + nc.ni * nc.ni;

    f2 = (nc.nr + 1.0);
    f2 = f2 * f2 + nc.ni * nc.ni;

    sqrtF = sqrt(f1 / f2);

    return (1.0 + sqrtF) / (1.0 - sqrtF);
}

/* Calculate the ideal reflected ray direction (independent of the brdf) */
Vector3D IdealReflectedDirection(Vector3D *in, Vector3D *normal) {
    double tmp;
    Vector3D result;

    tmp = 2 * VECTORDOTPRODUCT(*normal, *in);
    VECTORSCALE(tmp, *normal, result);
    VECTORSUBTRACT(*in, result, result);
    VECTORNORMALIZE(result);

    return result;
}

static Vector3Dd DIdealReflectedDirection(Vector3Dd *in, Vector3Dd *normal) {
    double tmp;
    Vector3Dd result;

    tmp = 2.0 * VECTORDOTPRODUCT(*normal, *in);
    VECTORSCALE(tmp, *normal, result);
    VECTORSUBTRACT(*in, result, result);
    VECTORNORMALIZE(result);

    return result;
}


/* Calculate the perfect refracted ray direction.
 * Sets totalInternalReflection to true or false accordingly.
 * Cfr. An Introduction to Global_Raytracer_activeRaytracer (Glassner)
 */

Vector3D IdealRefractedDirection(Vector3D *in, Vector3D *normal,
                                 REFRACTIONINDEX inIndex,
                                 REFRACTIONINDEX outIndex,
                                 int *totalInternalReflection) {
    double Ci, Ct, Ct2;
    double refractionIndex;
    /*  float nr, ni; */
    double normalScale;
    Vector3D result;

    /* !! Only real part of n for now */

    refractionIndex = inIndex.nr / outIndex.nr;

    Ci = -VECTORDOTPRODUCT(*in, *normal);
    Ct2 = 1 + refractionIndex * refractionIndex * (Ci * Ci - 1);

    if ( Ct2 < 0 ) {
        *totalInternalReflection = true;
        return (IdealReflectedDirection(in, normal));
    }
    *totalInternalReflection = false;

    Ct = sqrt(Ct2);

    normalScale = refractionIndex * Ci - Ct;

    VECTORSCALE(refractionIndex, *in, result);
    VECTORSUMSCALED(result, normalScale, *normal, result);

    /* Normalise */
    VECTORNORMALIZE(result);

    return result;
}


Vector3Dd DIdealRefractedDirection(Vector3Dd *in, Vector3Dd *normal,
                                   REFRACTIONINDEX inIndex,
                                   REFRACTIONINDEX outIndex,
                                   int *totalInternalReflection) {
    double Ci, Ct, Ct2;
    double refractionIndex;
    /* float nr, ni; */
    double normalScale;
    Vector3Dd result;

    /* !! Only real part of n for now */
    refractionIndex = inIndex.nr / outIndex.nr;

    Ci = -VECTORDOTPRODUCT(*in, *normal);
    Ct2 = 1 + refractionIndex * refractionIndex * (Ci * Ci - 1);

    if ( Ct2 < 0 ) {
        *totalInternalReflection = true;
        return (DIdealReflectedDirection(in, normal));
    }
    *totalInternalReflection = false;

    Ct = sqrt(Ct2);

    normalScale = refractionIndex * Ci - Ct;

    VECTORSCALE(refractionIndex, *in, result);
    VECTORSUMSCALED(result, normalScale, *normal, result);

    /* Normalise */
    VECTORNORMALIZE(result);

    return result;
}

