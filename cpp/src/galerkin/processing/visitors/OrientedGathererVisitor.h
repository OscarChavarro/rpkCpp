#ifndef ORIENTED_GATHERER_VISITOR__
#define ORIENTED_GATHERER_VISITOR__

#include "galerkin/processing/visitors/ClusterLeafVisitor.h"
class OrientedGathererVisitor final : public ClusterLeafVisitor {
  private:
    Interaction *link;
    ColorRgb *sourceRadiance;

  public:
    OrientedGathererVisitor(Interaction *inLink, ColorRgb *inSourceRadiance);
    ~OrientedGathererVisitor() final;

    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState) final;
};

#endif
