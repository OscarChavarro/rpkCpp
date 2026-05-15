#ifndef __ORIENTED_GATHERER_VISITOR__
#define __ORIENTED_GATHERER_VISITOR__

#include "galerkin/processing/visitors/ClusterLeafVisitor.h"
class OrientedGathererVisitor: public ClusterLeafVisitor{ private:
    Interaction *link;
    ColorRgb *sourceRadiance;

  public:
    OrientedGathererVisitor(Interaction *inLink, ColorRgb *inSourceRadiance);
    ~OrientedGathererVisitor();

    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState);
};

#endif
