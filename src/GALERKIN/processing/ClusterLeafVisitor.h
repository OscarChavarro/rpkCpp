#ifndef __CLUSTER_LEAF_VISITOR__
#define __CLUSTER_LEAF_VISITOR__

class ClusterLeafVisitor {
  public:
    virtual void visit(GalerkinElement *elem, const GalerkinState *galerkinState, ColorRgb *accumulatedRadiance) const = 0;
    virtual ~ClusterLeafVisitor() {}
};

#endif
