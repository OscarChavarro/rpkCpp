#include "galerkin/processing/visitors/PowerAccumulatorVisitor.h"

PowerAccumulatorVisitor::PowerAccumulatorVisitor(
    ColorRgb inSourceRadiance,
    Vector3D inSamplePoint)
{
    sourceRadiance = inSourceRadiance;
    samplePoint = inSamplePoint;
    accumulatedRadiance.clear();
}

PowerAccumulatorVisitor::~PowerAccumulatorVisitor() {
}

ColorRgb
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
    Vector3D dir;
    ColorRgb rad;

    dir.subtraction(samplePoint, galerkinElement->patch->midPoint);
    dist = dir.norm();
    if ( dist < Numeric::EPSILON ) {
        srcOs = 1.0f;
    } else {
        srcOs = dir.dotProduct(galerkinElement->patch->normal) / dist;
    }
    if ( srcOs <= 0.0f ) {
        // Receiver point is behind the src
        return;
    }

    if ( galerkinState->galerkinIterationMethod == GalerkinIterationMethod::GAUSS_SEIDEL ||
         galerkinState->galerkinIterationMethod == GalerkinIterationMethod::JACOBI ) {
        rad = galerkinElement->radiance[0];
    } else {
        rad = galerkinElement->unShotRadiance[0];
    }

    accumulatedRadiance.addScaled(sourceRadiance, srcOs * galerkinElement->area, rad);
}
