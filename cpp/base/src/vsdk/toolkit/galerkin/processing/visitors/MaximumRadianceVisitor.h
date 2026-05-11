#ifndef MAXIMUM_RADIANCE_VISITOR__
#define MAXIMUM_RADIANCE_VISITOR__

#include "vsdk/toolkit/galerkin/processing/visitors/ClusterLeafVisitor.h"

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
