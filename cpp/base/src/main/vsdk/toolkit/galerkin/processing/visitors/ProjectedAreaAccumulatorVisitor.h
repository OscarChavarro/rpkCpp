#ifndef PROJECTED_AREA_ACCUMULATOR_VISITOR__
#define PROJECTED_AREA_ACCUMULATOR_VISITOR__

#include "vsdk/toolkit/galerkin/processing/visitors/ClusterLeafVisitor.h"

class ProjectedAreaAccumulatorVisitor final : public ClusterLeafVisitor {
  private:
    double totalProjectedArea;

  public:
    ProjectedAreaAccumulatorVisitor();
    ~ProjectedAreaAccumulatorVisitor() final;

    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState) final;
    double getTotalProjectedArea() const;
};

#endif
