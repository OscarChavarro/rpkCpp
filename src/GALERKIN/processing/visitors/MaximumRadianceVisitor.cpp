#include "GALERKIN/processing/visitors/MaximumRadianceVisitor.h"

MaximumRadianceVisitor::MaximumRadianceVisitor() {
    accumulatedRadiance.clear();
}

MaximumRadianceVisitor::~MaximumRadianceVisitor() {
}

ColorRgb
MaximumRadianceVisitor::getAccumulatedRadiance() const {
    return accumulatedRadiance;
}

void
MaximumRadianceVisitor::visit(
    GalerkinElement *galerkinElement,
    const GalerkinState *galerkinState)
{
    ColorRgb rad;
    if ( galerkinState->galerkinIterationMethod == GAUSS_SEIDEL ||
         galerkinState->galerkinIterationMethod == JACOBI ) {
        rad = galerkinElement->radiance[0];
    } else {
        rad = galerkinElement->unShotRadiance[0];
    }
    accumulatedRadiance.maximum(accumulatedRadiance, rad);
}
