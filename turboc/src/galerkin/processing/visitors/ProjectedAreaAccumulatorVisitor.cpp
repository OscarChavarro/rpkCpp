#include "galerkin/processing/ClusterTraversalStrategy.h"
#include "galerkin/processing/visitors/ProjectedAreaAccumulatorVisitor.h"

ProjectedAreaAccumulatorVisitor::ProjectedAreaAccumulatorVisitor() {
    totalProjectedArea = 0.0;
}

ProjectedAreaAccumulatorVisitor::~ProjectedAreaAccumulatorVisitor() {
}

void
ProjectedAreaAccumulatorVisitor::visit(
    GalerkinElement *galerkinElement,
    const GalerkinState * /*galerkinState*/) {
    totalProjectedArea += ClusterTraversalStrategy::srfcPrjctAreaTSmplPnt(galerkinElement);
}

double
ProjectedAreaAccumulatorVisitor::getTotalProjectedArea() const {
    return totalProjectedArea;
}
