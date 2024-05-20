#ifndef __PROJECTED_AREA_ACCUMULATOR_VISITOR__
#define __PROJECTED_AREA_ACCUMULATOR_VISITOR__

#include "GALERKIN/processing/visitors/ClusterLeafVisitor.h"

class ProjectedAreaAccumulatorVisitor final : public ClusterLeafVisitor {
private:
    double totalProjectedArea;

  public:
    ProjectedAreaAccumulatorVisitor();
    ~ProjectedAreaAccumulatorVisitor() final;

    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState, ColorRgb *accumulatedRadiance) final;
    double getTotalProjectedArea() const;
};

#endif
