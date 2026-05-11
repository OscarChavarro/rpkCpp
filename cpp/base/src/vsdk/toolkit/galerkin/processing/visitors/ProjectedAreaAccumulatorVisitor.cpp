#include "vsdk/toolkit/galerkin/processing/ClusterTraversalStrategy.h"
#include "vsdk/toolkit/galerkin/processing/visitors/ProjectedAreaAccumulatorVisitor.h"

ProjectedAreaAccumulatorVisitor::ProjectedAreaAccumulatorVisitor() {
    totalProjectedArea = 0.0;
}

ProjectedAreaAccumulatorVisitor::~ProjectedAreaAccumulatorVisitor() {
}

void
ProjectedAreaAccumulatorVisitor::visit(
    GalerkinElement *galerkinElement,
    const GalerkinState * /*galerkinState*/) {
    totalProjectedArea += ClusterTraversalStrategy::surfaceProjectedAreaToSamplePoint(galerkinElement);
}

double
ProjectedAreaAccumulatorVisitor::getTotalProjectedArea() const {
    return totalProjectedArea;
}
