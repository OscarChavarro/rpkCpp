#include "vsdk/toolkit/material/PhongEmittanceDistributionFunction.h"

PhongEmittanceDistributionFunction::PhongEmittanceDistributionFunction(const PhongMaterial *inMaterial):
    material(inMaterial)
{
}

PhongEmittanceDistributionFunction::~PhongEmittanceDistributionFunction() {
}

bool
PhongEmittanceDistributionFunction::edfIsTextured() {
    return false;
}

/**
No emitter defines its own shading frame: callers build a default one
*/
bool
PhongEmittanceDistributionFunction::edfShadingFrame(
    const ShadingContext & /*context*/,
    const Vector3Df * /*X*/,
    const Vector3Df * /*Y*/,
    const Vector3Df * /*Z*/) {
    return false;
}

ColorRgb
PhongEmittanceDistributionFunction::phongEmittance(const ShadingContext * /*context*/, const char flags) const {
    return material->emittance(flags);
}

ColorRgb
PhongEmittanceDistributionFunction::phongEdfEval(
    const ShadingContext *context,
    const Vector3Df *out,
    const char flags,
    double *probabilityDensityFunction) const
{
    if ( context == nullptr ) {
        return material->emissionEvaluate(nullptr, *out, flags, probabilityDensityFunction);
    }
    const SurfaceShadingPoint point = context->toShadingPoint();
    return material->emissionEvaluate(&point, *out, flags, probabilityDensityFunction);
}

Vector3Df
PhongEmittanceDistributionFunction::phongEdfSample(
    const ShadingContext *context,
    const char flags,
    const double xi1,
    const double xi2,
    ColorRgb *selfEmittedRadiance,
    double *probabilityDensityFunction) const
{
    if ( context == nullptr ) {
        return material->emissionSample(
            nullptr, flags, xi1, xi2, selfEmittedRadiance, probabilityDensityFunction);
    }
    const SurfaceShadingPoint point = context->toShadingPoint();
    return material->emissionSample(
        &point, flags, xi1, xi2, selfEmittedRadiance, probabilityDensityFunction);
}
