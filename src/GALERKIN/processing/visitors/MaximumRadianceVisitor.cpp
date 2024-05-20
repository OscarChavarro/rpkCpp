#include "GALERKIN/processing/visitors/MaximumRadianceVisitor.h"

MaximumRadianceVisitor::MaximumRadianceVisitor() {
}

MaximumRadianceVisitor::~MaximumRadianceVisitor() {
}

void
MaximumRadianceVisitor::visit(
    GalerkinElement *galerkinElement,
    const GalerkinState *galerkinState,
    ColorRgb *accumulatedRadiance) const
{
    ColorRgb rad;
    if ( galerkinState->galerkinIterationMethod == GAUSS_SEIDEL ||
         galerkinState->galerkinIterationMethod == JACOBI ) {
        rad = galerkinElement->radiance[0];
    } else {
        rad = galerkinElement->unShotRadiance[0];
    }
    accumulatedRadiance->maximum(*accumulatedRadiance, rad);
}
