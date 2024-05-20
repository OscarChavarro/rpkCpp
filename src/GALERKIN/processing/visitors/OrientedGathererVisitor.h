#ifndef __ORIENTED_GATHERER_VISITOR__
#define __ORIENTED_GATHERER_VISITOR__

#include "GALERKIN/processing/visitors/ClusterLeafVisitor.h"

class OrientedGathererVisitor final : public ClusterLeafVisitor {
  private:
    Interaction *link;
    ColorRgb *sourceRadiance;

  public:
    OrientedGathererVisitor(Interaction *inLink, ColorRgb *inSourceRadiance);
    ~OrientedGathererVisitor() final;

    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState, ColorRgb *accumulatedRadiance) final;
};

#endif
