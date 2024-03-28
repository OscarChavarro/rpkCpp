#include <cstdlib>

#include "common/error.h"
#include "material/PhongEmittanceDistributionFunctions.h"
#include "material/spherical.h"

/**
Creates a EDF instance with given data and methods. A pointer
to the created EDF object is returned
*/
PhongEmittanceDistributionFunctions *
edfCreate(PHONG_EDF *data) {
    PhongEmittanceDistributionFunctions *edf = (PhongEmittanceDistributionFunctions *)malloc(sizeof(PhongEmittanceDistributionFunctions));
    edf->data = data;

    return edf;
}

/**
Returns emittance, reflectance, transmittance
*/
static COLOR
phongEmittance(PHONG_EDF *edf, RayHit * /*hit*/, XXDFFLAGS flags) {
    COLOR result;

    colorClear(result);

    if ( flags & DIFFUSE_COMPONENT ) {
        colorAdd(result, edf->Kd, result);
    }

    if ( PHONG_IS_SPECULAR(*edf)) {
        if ( flags & SPECULAR_COMPONENT ) {
            colorAdd(result, edf->Ks, result);
        }
    } else {
        if ( flags & GLOSSY_COMPONENT ) {
            colorAdd(result, edf->Ks, result);
        }
    }

    return result;
}

/**
Returns the emittance (self-emitted radiant exitance) [W / m ^ 2] of the EDF
*/
COLOR
edfEmittance(PhongEmittanceDistributionFunctions *edf, RayHit *hit, char flags) {
    if ( edf != nullptr ) {
        return phongEmittance(edf->data, hit, flags);
    } else {
        static COLOR emit;
        colorClear(emit);
        return emit;
    }
}

bool
edfIsTextured(PhongEmittanceDistributionFunctions * /*edf*/) {
    return false;
}

/**
Edf evaluations
*/
static COLOR
phongEdfEval(PHONG_EDF *edf, RayHit *hit, Vector3D *out, XXDFFLAGS flags, double *probabilityDensityFunction) {
    Vector3D normal;
    COLOR result;
    double cosL;

    colorClear(result);
    if ( probabilityDensityFunction ) {
        *probabilityDensityFunction = 0.0;
    }

    if ( !hitShadingNormal(hit, &normal) ) {
        logWarning("phongEdfEval", "Couldn't determine shading normal");
        return result;
    }

    cosL = vectorDotProduct(*out, normal);

    if ( cosL < 0.0 ) {
        return result;
    } // Back face of a light does not radiate

    // kd + ks (idealReflected * out)^n

    if ( flags & DIFFUSE_COMPONENT ) {
        // Divide by PI to turn radiant exitance [W/m^2] into exitant radiance [W/m^2 sr]
        colorAdd(result, edf->kd, result);
        if ( probabilityDensityFunction ) {
            *probabilityDensityFunction = cosL / M_PI;
        }
    }

    if ( flags & SPECULAR_COMPONENT ) {
        // ???
    }

    return result;
}

/**
Evaluates the edf: return exitant radiance [W/m^2 sr] into the direction
out. If probabilityDensityFunction is not null, the stochasticJacobiProbability density of the direction is
computed and returned in probabilityDensityFunction
*/
COLOR
edfEval(PhongEmittanceDistributionFunctions *edf, RayHit *hit, Vector3D *out, XXDFFLAGS flags, double *probabilityDensityFunction) {
    if ( edf != nullptr ) {
        return phongEdfEval(edf->data, hit, out, flags, probabilityDensityFunction);
    } else {
        static COLOR val;
        colorClear(val);
        if ( probabilityDensityFunction ) {
            *probabilityDensityFunction = 0.0;
        }
        return val;
    }
}

/**
Edf sampling
*/
static Vector3D
phongEdfSample(
    PHONG_EDF *edf,
    RayHit *hit,
    XXDFFLAGS flags,
    double xi1,
    double xi2,
    COLOR *selfEmittedRadiance,
    double *probabilityDensityFunction)
{
    Vector3D dir = {0.0, 0.0, 1.0};
    if ( selfEmittedRadiance ) {
        colorClear(*selfEmittedRadiance);
    }
    if ( probabilityDensityFunction ) {
        *probabilityDensityFunction = 0.0;
    }

    if ( flags & DIFFUSE_COMPONENT ) {
        double sProbabilityDensityFunction;
        CoordSys coord;

        Vector3D normal;
        if ( !hitShadingNormal(hit, &normal)) {
            logWarning("phongEdfEval", "Couldn't determine shading normal");
            return dir;
        }

        vectorCoordSys(&normal, &coord);
        dir = sampleHemisphereCosTheta(&coord, xi1, xi2, &sProbabilityDensityFunction);
        if ( probabilityDensityFunction ) {
            *probabilityDensityFunction = sProbabilityDensityFunction;
        }
        if ( selfEmittedRadiance ) {
            colorScale((1.0f / (float)M_PI), edf->Kd, *selfEmittedRadiance);
        }
    }

    return dir;
}

/**
Samples a direction according to the specified edf and emission range determined
by flags. If emitted_radiance is not a null pointer, the emitted radiance along
the generated direction is returned. If probabilityDensityFunction is not null, the stochasticJacobiProbability density
of the generated direction is computed and returned in probabilityDensityFunction
*/
Vector3D
edfSample(
    PhongEmittanceDistributionFunctions *edf,
    RayHit *hit,
    char flags,
    double xi1,
    double xi2,
    COLOR *emittedRadiance,
    double *probabilityDensityFunction)
{
    if ( edf != nullptr ) {
        return phongEdfSample(edf->data, hit, flags, xi1, xi2, emittedRadiance, probabilityDensityFunction);
    } else {
        Vector3D v = {0.0, 0.0, 0.0};
        logFatal(-1, "edfSample", "Can't sample EDF");
        return v;
    }
}

/**
Computes a shading frame at the given hit point. The Z axis of this frame is
the shading normal, The X axis is in the tangent plane on the surface at the
hit point ("brush" direction relevant for anisotropic shaders e.g.). Y
is perpendicular to X and Z. X and Y may be null pointers. In this case,
only the shading normal is returned, avoiding computation of the X and
Y axis if possible).
Note: also edf's can have a routine for computing the shading frame. If a
material has both an edf and a bsdf, the shading frame shall of course
be the same.
This routine returns TRUE if a shading frame could be constructed and FALSE if
not. In the latter case, a default frame needs to be used (not computed by this
routine - hitPointShadingFrame() in material.[ch] constructs such a frame if
needed)
*/
bool
edfShadingFrame(PhongEmittanceDistributionFunctions * /*edf*/, RayHit * /*hit*/, Vector3D * /*X*/, Vector3D * /*Y*/, Vector3D * /*Z*/) {
    return true;
}
