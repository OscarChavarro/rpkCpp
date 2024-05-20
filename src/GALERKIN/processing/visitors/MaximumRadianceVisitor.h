#ifndef __MAXIMUM_RADIANCE_VISITOR__
#define __MAXIMUM_RADIANCE_VISITOR__

#include "GALERKIN/processing/visitors/ClusterLeafVisitor.h"

class MaximumRadianceVisitor final : public ClusterLeafVisitor {
  private:
    ColorRgb accumulatedRadiance;
  public:
    MaximumRadianceVisitor();
    ~MaximumRadianceVisitor() final;
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState) final;
    ColorRgb getAccumulatedRadiance() const;
};

#endif
