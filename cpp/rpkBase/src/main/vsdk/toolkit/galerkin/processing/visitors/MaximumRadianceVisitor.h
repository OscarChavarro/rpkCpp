#ifndef __MAXIMUM_RADIANCE_VISITOR__
#define __MAXIMUM_RADIANCE_VISITOR__

#include "vsdk/toolkit/galerkin/processing/visitors/ClusterLeafVisitor.h"

class MaximumRadianceVisitor final : public ClusterLeafVisitor {
  private:
    double accumulatedRadiance[3];
  public:
    MaximumRadianceVisitor();
    ~MaximumRadianceVisitor() final;
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState) final;
    ColorRgb getAccumulatedRadiance() const;
};

#endif
