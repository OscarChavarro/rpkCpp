#ifndef RPK_ORIENTEDGATHERERVISITOR_H
#define RPK_ORIENTEDGATHERERVISITOR_H

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
