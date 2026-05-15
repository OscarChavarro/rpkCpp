#include "common/logging/Logger.h"
#include "material/PhongBidirectionalReflectanceDistributionFunction.h"
#include "material/PhongEmittanceDistributionFunction.h"
#include "material/Xxdf.h"

bool
PhongEmitDistFunc::isSpecular() const {
    return Ns >= PHONG_LOWEST_SPECULAR_EXP;
}

/**
Creates Phong type EDF, BRDF, BTDF data structs:
Kd = diffuse emittance [W/m^2], reflectance or transmittance (number between 0 and 1)
Ks = specular emittance, reflectance or transmittance (same dimensions as Kd)
Ns = Phong exponent.
note: Emittance is total power emitted by the light source per unit of area
*/
PhongEmitDistFunc::PhongEmitDistFunc(
    const ColorRgb *KdParameter,
    const ColorRgb *KsParameter,
    double NsParameter)
{
    Kd = *KdParameter;
    kd.scaledCopy((1.00f / ((float)(M_PI))), Kd); // Because we use it often
    Ks = *KsParameter;
    if ( !Ks.isBlack() ) {
        Logger::warning("phongEdfCreate", "Non-diffuse light sources not yet implemented");
    }
    Ns = ((float)(NsParameter));
}

PhongEmitDistFunc::~PhongEmitDistFunc() {
}

/**
Returns emittance, reflectance, transmittance
*/
ColorRgb
PhongEmitDistFunc::phongEmittance(const ShadingContext & /*context*/, const char flags) const {
    ColorRgb result;

    result.clear();
    if ( flags & DIFFUSE_COMPONENT ) {
        result.add(result, Kd);
    }

    if ( isSpecular() ) {
        if ( flags & SPECULAR_COMPONENT ) {
            result.add(result, Ks);
        }
    } else {
        if ( flags & GLOSSY_COMPONENT ) {
            result.add(result, Ks);
        }
    }

    return result;
}

ColorRgb
PhongEmitDistFunc::phongEmittance(const RayHit * /*hit*/, const char flags) const {
    ShadingContext context(
        Vector3D(),
        Vector3D(),
        Vector3D(),
        Vector3D(),
        Vector2Dd(),
        CoordinateSystem(),
        NULL,
        0);
    return phongEmittance(context, flags);
}

/**
Returns the emittance (self-emitted radiant exitance) [W / m ^ 2] of the EDF
*/

bool
PhongEmitDistFunc::edfIsTextured() {
    return false;
}

/**
Evaluates the edf: return exitant radiance [W/m^2 sr] into the direction
out. If probabilityDensityFunction is not null, the stochasticJacobiProbability density of the direction is
computed and returned in probabilityDensityFunction
*/
ColorRgb
PhongEmitDistFunc::phongEdfEval(
    const ShadingContext &context,
    const Vector3D *out,
    char flags,
    double *probabilityDensityFunction) const
{
    Vector3D normal;
    ColorRgb result;
    double cosL;

    result.clear();
    if ( probabilityDensityFunction ) {
        *probabilityDensityFunction = 0.0;
    }

    if ( !context.hasFlag(NORMAL) ) {
        Logger::warning("phongEdfEval", "Couldn't determine shading normal");
        return result;
    }
    normal = context.getShadingNormal();

    cosL = out->dotProduct(normal);

    if ( cosL < 0.0 ) {
        return result;
    } // Back face of a light does not radiate

    // kd + ks (idealReflected * out) ^ n

    if ( flags & DIFFUSE_COMPONENT ) {
        // Divide by PI to turn radiant exitance [W / m ^ 2] into exitant radiance [W / m ^ 2 sr]
        result.add(result, kd);
        if ( probabilityDensityFunction ) {
            *probabilityDensityFunction = cosL / M_PI;
        }
    }

    if ( flags & SPECULAR_COMPONENT ) {
        // ???
    }

    return result;
}

ColorRgb
PhongEmitDistFunc::phongEdfEval(
    RayHit *hit,
    const Vector3D *out,
    char flags,
    double *probabilityDensityFunction) const
{
    if ( hit == NULL ) {
        if ( probabilityDensityFunction ) {
            *probabilityDensityFunction = 0.0;
        }
        ColorRgb result;
        result.clear();
        return result;
    }
    Vector3D normal;
    if ( !hit->shadingNormal(&normal) ) {
        if ( probabilityDensityFunction ) {
            *probabilityDensityFunction = 0.0;
        }
        ColorRgb result;
        result.clear();
        return result;
    }
    Vector3D texCoord;
    unsigned int localFlags = NORMAL;
    if ( hit->getTexCoord(&texCoord) ) {
        localFlags |= TEXTURE_COORDINATE;
    } else {
        texCoord.set(0.0, 0.0, 0.0);
    }
    ShadingContext context(
        hit->getPoint(),
        hit->getGeometricNormal(),
        normal,
        texCoord,
        hit->getUv(),
        hit->getShadingFrame(),
        hit->getMaterial(),
        localFlags);
    return phongEdfEval(context, out, flags, probabilityDensityFunction);
}

/**
Samples a direction according to the specified edf and emission range determined
by flags. If emitted_radiance is not a null pointer, the emitted radiance along
the generated direction is returned. If probabilityDensityFunction is not null, the stochasticJacobiProbability density
of the generated direction is computed and returned in probabilityDensityFunction
*/
Vector3D
PhongEmitDistFunc::phongEdfSample(
    const ShadingContext &context,
    char flags,
    double xi1,
    double xi2,
    ColorRgb *selfEmittedRadiance,
    double *probabilityDensityFunction) const
{
    if ( selfEmittedRadiance ) {
        selfEmittedRadiance->clear();
    }
    if ( probabilityDensityFunction ) {
        *probabilityDensityFunction = 0.0;
    }

    Vector3D dir(0.0, 0.0, 1.0);
    if ( flags & DIFFUSE_COMPONENT ) {
        double sProbabilityDensityFunction;
        CoordinateSystem coord;

        if ( !context.hasFlag(NORMAL) ) {
            Logger::warning("phongEdfEval", "Couldn't determine shading normal");
            return dir;
        }
        Vector3D normal = context.getShadingNormal();

        // Section [ARVO1995b].2: two independent samples (xi1, xi2) in [0,1]^2
        // are mapped to a direction on the hemisphere after building a local frame.
        coord.setFromZAxis(&normal);
        dir = coord.sampleHemisphereCosTheta(xi1, xi2, &sProbabilityDensityFunction);
        if ( probabilityDensityFunction ) {
            *probabilityDensityFunction = sProbabilityDensityFunction;
        }
        if ( selfEmittedRadiance ) {
            selfEmittedRadiance->scaledCopy((1.0f / ((float)(M_PI))), Kd);
        }
    }

    return dir;
}

Vector3D
PhongEmitDistFunc::phongEdfSample(
    RayHit *hit,
    char flags,
    double xi1,
    double xi2,
    ColorRgb *selfEmittedRadiance,
    double *probabilityDensityFunction) const
{
    if ( hit == NULL ) {
        if ( selfEmittedRadiance ) {
            selfEmittedRadiance->clear();
        }
        if ( probabilityDensityFunction ) {
            *probabilityDensityFunction = 0.0;
        }
        Vector3D dir(0.0, 0.0, 1.0);
        return dir;
    }
    Vector3D normal;
    if ( !hit->shadingNormal(&normal) ) {
        if ( selfEmittedRadiance ) {
            selfEmittedRadiance->clear();
        }
        if ( probabilityDensityFunction ) {
            *probabilityDensityFunction = 0.0;
        }
        Vector3D dir(0.0, 0.0, 1.0);
        return dir;
    }
    Vector3D texCoord;
    unsigned int localFlags = NORMAL;
    if ( hit->getTexCoord(&texCoord) ) {
        localFlags |= TEXTURE_COORDINATE;
    } else {
        texCoord.set(0.0, 0.0, 0.0);
    }
    ShadingContext context(
        hit->getPoint(),
        hit->getGeometricNormal(),
        normal,
        texCoord,
        hit->getUv(),
        hit->getShadingFrame(),
        hit->getMaterial(),
        localFlags);
    return phongEdfSample(context, flags, xi1, xi2, selfEmittedRadiance, probabilityDensityFunction);
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
routine - pointShadingFrame() in material.[ch] constructs such a frame if
needed)
*/
bool
PhongEmitDistFunc::edfShadingFrame(
    const ShadingContext & /*context*/,
    const Vector3D * /*X*/,
    const Vector3D * /*Y*/,
    const Vector3D * /*Z*/) {
    return false;
}

bool
PhongEmitDistFunc::edfShadingFrame(
    const RayHit * /*hit*/,
    const Vector3D * /*X*/,
    const Vector3D * /*Y*/,
    const Vector3D * /*Z*/) {
    return false;
}
