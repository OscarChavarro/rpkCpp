#ifndef __MAXIMUM_RADIANCE_VISITOR__
#define __MAXIMUM_RADIANCE_VISITOR__

#include "galerkin/processing/visitors/ClusterLeafVisitor.h"

class MaximumRadianceVisitor: public ClusterLeafVisitor{ private:
    ColorRgb accumulatedRadiance;
  public:
    MaximumRadianceVisitor();
    ~MaximumRadianceVisitor();
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState);
    ColorRgb getAccumulatedRadiance() const;
};

#endif
