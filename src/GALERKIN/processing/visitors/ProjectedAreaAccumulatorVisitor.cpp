#include "GALERKIN/processing/visitors/ProjectedAreaAccumulatorVisitor.h"
#include "GALERKIN/processing/ClusterTraversalStrategy.h"

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
