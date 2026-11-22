#ifndef ORIENTED_GATHERER_VISITOR__
#define ORIENTED_GATHERER_VISITOR__

#include "vsdk/toolkit/galerkin/processing/visitors/ClusterLeafVisitor.h"
class OrientedGathererVisitor final : public ClusterLeafVisitor {
  private:
    Interaction *link;
    ColorRgbMutable *sourceRadiance;

  public:
    OrientedGathererVisitor(Interaction *inLink, ColorRgbMutable *inSourceRadiance);
    ~OrientedGathererVisitor() final;

    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState) final;
};

#endif
