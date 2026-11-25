#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/galerkin/processing/visitors/PowerAccumulatorVisitor.h"

PowerAccumulatorVisitor::PowerAccumulatorVisitor(
    const ColorRgb &inSourceRadiance,
    Vector3Df inSamplePoint)
{
    sourceRadiance[0] = inSourceRadiance.getR();
    sourceRadiance[1] = inSourceRadiance.getG();
    sourceRadiance[2] = inSourceRadiance.getB();
    samplePoint = inSamplePoint;
    accumulatedRadiance[0] = 0.0;
    accumulatedRadiance[1] = 0.0;
    accumulatedRadiance[2] = 0.0;
}

PowerAccumulatorVisitor::~PowerAccumulatorVisitor() {
}

ColorRgb
PowerAccumulatorVisitor::getAccumulatedRadiance() const {
    return ColorRgb(accumulatedRadiance[0], accumulatedRadiance[1], accumulatedRadiance[2]);
}

/**
Accumulates this leaf contribution towards the visitor sample point.
The contribution is weighted by projected area (cosine term and patch area),
uses the radiance channel that matches the current Galerkin iteration mode,
and ignores intra-cluster visibility.
*/
void
PowerAccumulatorVisitor::visit(
    GalerkinElement *galerkinElement,
    const GalerkinState *galerkinState)
{
    float srcOs;
    float dist;
    Vector3Df dir;

    dir = samplePoint.subtract(galerkinElement->patch->midPoint());
    dist = dir.length();
    if ( dist < VSDK::EPSILON ) {
        srcOs = 1.0F;
    } else {
        srcOs = dir.dotProduct(galerkinElement->patch->getNormal()) / dist;
    }
    if ( srcOs <= 0.0F ) {
        // Receiver point is behind the src
        return;
    }

    const auto *source =
        galerkinState->galerkinIterationMethod == GalerkinIterationMethod::GAUSS_SEIDEL ||
        galerkinState->isJacobi()
        ? &galerkinElement->radiance[0]
        : &galerkinElement->unShotRadiance[0];
    const double scale = srcOs * galerkinElement->area;
    accumulatedRadiance[0] = sourceRadiance[0] + scale * source->getR();
    accumulatedRadiance[1] = sourceRadiance[1] + scale * source->getG();
    accumulatedRadiance[2] = sourceRadiance[2] + scale * source->getB();
}
