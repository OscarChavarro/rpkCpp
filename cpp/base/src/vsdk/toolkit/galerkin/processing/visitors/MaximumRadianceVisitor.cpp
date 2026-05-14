#include "vsdk/toolkit/galerkin/processing/visitors/MaximumRadianceVisitor.h"
MaximumRadianceVisitor::MaximumRadianceVisitor() {
    accumulatedRadiance.clear();
}

MaximumRadianceVisitor::~MaximumRadianceVisitor() {
}

ColorRgbMutable
MaximumRadianceVisitor::getAccumulatedRadiance() const {
    return accumulatedRadiance;
}

void
MaximumRadianceVisitor::visit(
    GalerkinElement *galerkinElement,
    const GalerkinState *galerkinState)
{
    ColorRgbMutable rad(0.0, 0.0, 0.0);
    if ( galerkinState->galerkinIterationMethod == GalerkinIterationMethod::GAUSS_SEIDEL ||
         galerkinState->galerkinIterationMethod == GalerkinIterationMethod::JACOBI ) {
        rad = galerkinElement->radiance[0];
    } else {
        rad = galerkinElement->unShotRadiance[0];
    }
    accumulatedRadiance.maximum(accumulatedRadiance, rad);
}
