#include "vsdk/toolkit/material/PhongBidirectionalScatteringDistributionFunction.h"

namespace {

const PhongMaterial *
mediumOf(const PhongBidirectionalScatteringDistributionFunction *bsdf) {
    return bsdf != nullptr ? bsdf->getMaterial() : nullptr;
}

}

PhongBidirectionalScatteringDistributionFunction::PhongBidirectionalScatteringDistributionFunction(
    const PhongMaterial *inMaterial):
    material(inMaterial)
{
}

PhongBidirectionalScatteringDistributionFunction::~PhongBidirectionalScatteringDistributionFunction() {
}

/**
No material defines its own shading frame: callers build a default one
*/
bool
PhongBidirectionalScatteringDistributionFunction::bsdfShadingFrame(
    const ShadingContext & /*context*/,
    const Vector3Df * /*X*/,
    const Vector3Df * /*Y*/,
    const Vector3Df * /*Z*/)
{
    return false;
}

/**
Scattered power (diffuse/glossy/specular reflectance and/or transmittance) according to flags
*/
ColorRgb
PhongBidirectionalScatteringDistributionFunction::splitBsdfScatteredPower(const ShadingContext &context, const char flags) const {
    const Vector3Df textureCoordinate = context.getTexCoord();
    return material->scatteredPower(
        flags, context.hasFlag(SHCTX_TEXTURE_COORDINATE) ? &textureCoordinate : nullptr);
}

bool
PhongBidirectionalScatteringDistributionFunction::splitBsdfIsTextured() const {
    return material->isTextured();
}

#ifdef RAYTRACING_ENABLED

void
PhongBidirectionalScatteringDistributionFunction::indexOfRefraction(RefractionIndex *index) const {
    index->set(material->getRefractionIndex(), material->getRefractionIndexImaginary());
}

Vector3Df
PhongBidirectionalScatteringDistributionFunction::sample(
    const ShadingContext &context,
    const PhongBidirectionalScatteringDistributionFunction *inBsdf,
    const PhongBidirectionalScatteringDistributionFunction *outBsdf,
    const Vector3Df *in,
    const int doRussianRoulette,
    const char flags,
    const double x1,
    const double x2,
    double *probabilityDensityFunction) const
{
    return material->scatteringSample(
        context.toShadingPoint(), mediumOf(inBsdf), mediumOf(outBsdf), *in,
        doRussianRoulette != 0, flags, x1, x2, probabilityDensityFunction);
}

ColorRgb
PhongBidirectionalScatteringDistributionFunction::evaluate(
    const ShadingContext &context,
    const PhongBidirectionalScatteringDistributionFunction *inBsdf,
    const PhongBidirectionalScatteringDistributionFunction *outBsdf,
    const Vector3Df *in,
    const Vector3Df *out,
    const char flags) const
{
    return material->scatteringEvaluate(
        context.toShadingPoint(), mediumOf(inBsdf), mediumOf(outBsdf),
        *in, *out, flags);
}

void
PhongBidirectionalScatteringDistributionFunction::evaluateProbabilityDensityFunction(
    const ShadingContext &context,
    const PhongBidirectionalScatteringDistributionFunction *inBsdf,
    const PhongBidirectionalScatteringDistributionFunction *outBsdf,
    const Vector3Df *in,
    const Vector3Df *out,
    const char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR) const
{
    material->scatteringDensity(
        context.toShadingPoint(), mediumOf(inBsdf), mediumOf(outBsdf),
        *in, *out, flags,
        probabilityDensityFunction, probabilityDensityFunctionRR);
}

ColorRgb
PhongBidirectionalScatteringDistributionFunction::bsdfEvalComponents(
    const ShadingContext &context,
    const PhongBidirectionalScatteringDistributionFunction *inBsdf,
    const PhongBidirectionalScatteringDistributionFunction *outBsdf,
    const Vector3Df *in,
    const Vector3Df *out,
    const char flags,
    ColorRgb *colArray) const
{
    return material->scatteringEvaluateComponents(
        context.toShadingPoint(), mediumOf(inBsdf), mediumOf(outBsdf),
        *in, *out, flags, colArray);
}

#endif
