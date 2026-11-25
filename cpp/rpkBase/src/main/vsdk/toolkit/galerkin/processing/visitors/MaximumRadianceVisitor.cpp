#include "vsdk/toolkit/galerkin/processing/visitors/MaximumRadianceVisitor.h"
MaximumRadianceVisitor::MaximumRadianceVisitor(): accumulatedRadiance{0.0, 0.0, 0.0} {
}

MaximumRadianceVisitor::~MaximumRadianceVisitor() {
}

ColorRgb
MaximumRadianceVisitor::getAccumulatedRadiance() const {
    return ColorRgb(accumulatedRadiance[0], accumulatedRadiance[1], accumulatedRadiance[2]);
}

void
MaximumRadianceVisitor::visit(
    GalerkinElement *galerkinElement,
    const GalerkinState *galerkinState)
{
    const auto *sourceRadiance =
        galerkinState->galerkinIterationMethod == GalerkinIterationMethod::GAUSS_SEIDEL ||
        galerkinState->isJacobi()
        ? &galerkinElement->radiance[0]
        : &galerkinElement->unShotRadiance[0];
    const double sample[3] = {
        sourceRadiance->getR(), sourceRadiance->getG(), sourceRadiance->getB()};
    for ( int i = 0; i < 3; i++ ) {
        accumulatedRadiance[i] = accumulatedRadiance[i] > sample[i] ? accumulatedRadiance[i] : sample[i];
    }
}
