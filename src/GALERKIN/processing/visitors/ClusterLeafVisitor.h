#ifndef __CLUSTER_LEAF_VISITOR__
#define __CLUSTER_LEAF_VISITOR__

#include "GALERKIN/GalerkinElement.h"

class ClusterLeafVisitor {
  public:
    virtual void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState) = 0;
    virtual ~ClusterLeafVisitor() {}
};

#endif
