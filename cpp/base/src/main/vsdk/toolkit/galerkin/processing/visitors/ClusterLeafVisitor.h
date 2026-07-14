#ifndef CLUSTER_LEAF_VISITOR__
#define CLUSTER_LEAF_VISITOR__

#include "vsdk/toolkit/galerkin/GalerkinElement.h"
class ClusterLeafVisitor {
  public:
    virtual void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState) = 0;
    virtual ~ClusterLeafVisitor() {}
};

#endif
