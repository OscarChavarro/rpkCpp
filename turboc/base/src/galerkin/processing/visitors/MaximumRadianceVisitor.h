#ifndef __MAXIMUM_RADIANCE_VISITOR__
#define __MAXIMUM_RADIANCE_VISITOR__

#include "galerkin/processing/visitors/ClusterLeafVisitor.h"

class MaximumRadianceVisitor: public ClusterLeafVisitor{ private:
    ColorRgbMutable accumulatedRadiance;
  public:
    MaximumRadianceVisitor();
    ~MaximumRadianceVisitor();
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState);
    ColorRgbMutable getAccumulatedRadiance() const;
};

#endif
