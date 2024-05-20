#include "GALERKIN/processing/visitors/PowerAccumulatorVisitor.h"

PowerAccumulatorVisitor::PowerAccumulatorVisitor(
    GalerkinElement *inSourceElement,
    ColorRgb inSourceRadiance,
    Vector3D inSamplePoint)
{
    sourceElement = inSourceElement;
    sourceRadiance = inSourceRadiance;
    samplePoint = inSamplePoint;
}

PowerAccumulatorVisitor::~PowerAccumulatorVisitor() {
}

/**
Uses global variables globalSourceRadiance and globalSamplePoint: accumulates the
power emitted by the element towards the globalSamplePoint in globalSourceRadiance
only taking into account the surface orientation with respect to the
sample point, (ignores intra cluster visibility)
*/
void
PowerAccumulatorVisitor::visit(
    GalerkinElement *galerkinElement,
    const GalerkinState *galerkinState,
    ColorRgb *accumulatedRadiance) const
{
    float srcOs;
    float dist;
    Vector3D dir;
    ColorRgb rad;

    dir.subtraction(samplePoint, sourceElement->patch->midPoint);
    dist = dir.norm();
    if ( dist < Numeric::EPSILON ) {
        srcOs = 1.0f;
    } else {
        srcOs = dir.dotProduct(sourceElement->patch->normal) / dist;
    }
    if ( srcOs <= 0.0f ) {
        // Receiver point is behind the src
        return;
    }

    if ( galerkinState->galerkinIterationMethod == GAUSS_SEIDEL ||
         galerkinState->galerkinIterationMethod == JACOBI ) {
        rad = sourceElement->radiance[0];
    } else {
        rad = sourceElement->unShotRadiance[0];
    }

    accumulatedRadiance->addScaled(sourceRadiance, srcOs * sourceElement->area, rad);
}