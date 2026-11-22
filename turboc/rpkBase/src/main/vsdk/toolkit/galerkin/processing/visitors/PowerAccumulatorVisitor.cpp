#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/galerkin/processing/visitors/PowerAccumulatorVisitor.h"

PowerAccumulatorVisitor::PowerAccumulatorVisitor(
    ColorRgbMutable inSourceRadiance,
    Vector3Df inSamplePoint)
{
    sourceRadiance = inSourceRadiance;
    samplePoint = inSamplePoint;
    accumulatedRadiance.clear();
}

PowerAccumulatorVisitor::~PowerAccumulatorVisitor() {
}

ColorRgbMutable
PowerAccumulatorVisitor::getAccumulatedRadiance() const {
    return accumulatedRadiance;
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
    ColorRgbMutable rad;

    dir = samplePoint.subtract(galerkinElement->patch->midPoint);
    dist = dir.length();
    if ( dist < VSDK::EPSILON ) {
        srcOs = 1.0f;
    } else {
        srcOs = dir.dotProduct(galerkinElement->patch->normal) / dist;
    }
    if ( srcOs <= 0.0f ) {
        // Receiver point is behind the src
        return;
    }

    if ( galerkinState->galerkinIterationMethod == GAUSS_SEIDEL ||
         galerkinState->isJacobi() ) {
        rad = galerkinElement->radiance[0];
    } else {
        rad = galerkinElement->unShotRadiance[0];
    }

    accumulatedRadiance.addScaled(sourceRadiance, srcOs * galerkinElement->area, rad);
}
