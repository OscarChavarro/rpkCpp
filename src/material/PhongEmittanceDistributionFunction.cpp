#include "common/error.h"
#include "material/PhongEmittanceDistributionFunction.h"
#include "material/spherical.h"

/**
Creates Phong type EDF, BRDF, BTDF data structs:
Kd = diffuse emittance [W/m^2], reflectance or transmittance (number between 0 and 1)
Ks = specular emittance, reflectance or transmittance (same dimensions as Kd)
Ns = Phong exponent.
note: Emittance is total power emitted by the light source per unit of area
*/
PhongEmittanceDistributionFunction::PhongEmittanceDistributionFunction(ColorRgb *KdParameter, ColorRgb *KsParameter, double NsParameter) {
    Kd = *KdParameter;
    colorScale((1.00f / (float)M_PI), Kd, kd); // Because we use it often
    Ks = *KsParameter;
    if ( !colorNull(Ks) ) {
        logWarning("phongEdfCreate", "Non-diffuse light sources not yet implemented");
    }
    Ns = (float)NsParameter;
}

/**
Returns emittance, reflectance, transmittance
*/
static ColorRgb
phongEmittance(PhongEmittanceDistributionFunction *edf, RayHit * /*hit*/, char flags) {
    ColorRgb result;

    colorClear(result);

    if ( flags & DIFFUSE_COMPONENT ) {
        colorAdd(result, edf->Kd, result);
    }

    if ( PHONG_IS_SPECULAR(*edf) ) {
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
ColorRgb
edfEmittance(PhongEmittanceDistributionFunction *edf, RayHit *hit, char flags) {
    if ( edf != nullptr ) {
        return phongEmittance(edf, hit, flags);
    } else {
        static ColorRgb emit;
        colorClear(emit);
        return emit;
    }
}

bool
edfIsTextured(PhongEmittanceDistributionFunction * /*edf*/) {
    return false;
}

/**
Edf evaluations
*/
static ColorRgb
phongEdfEval(PhongEmittanceDistributionFunction *edf, RayHit *hit, Vector3D *out, char flags, double *probabilityDensityFunction) {
    Vector3D normal;
    ColorRgb result;
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
ColorRgb
edfEval(PhongEmittanceDistributionFunction *edf, RayHit *hit, Vector3D *out, char flags, double *probabilityDensityFunction) {
    if ( edf != nullptr ) {
        return phongEdfEval(edf, hit, out, flags, probabilityDensityFunction);
    } else {
        static ColorRgb val;
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
    PhongEmittanceDistributionFunction *edf,
    RayHit *hit,
    char flags,
    double xi1,
    double xi2,
    ColorRgb *selfEmittedRadiance,
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
        if ( !hitShadingNormal(hit, &normal) ) {
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
    PhongEmittanceDistributionFunction *edf,
    RayHit *hit,
    char flags,
    double xi1,
    double xi2,
    ColorRgb *emittedRadiance,
    double *probabilityDensityFunction)
{
    if ( edf != nullptr ) {
        return phongEdfSample(edf, hit, flags, xi1, xi2, emittedRadiance, probabilityDensityFunction);
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
edfShadingFrame(PhongEmittanceDistributionFunction * /*edf*/, RayHit * /*hit*/, Vector3D * /*X*/, Vector3D * /*Y*/, Vector3D * /*Z*/) {
    return false;
}