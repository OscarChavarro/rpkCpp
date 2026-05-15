#ifndef __ORIENTED_GATHERER_VISITOR__
#define __ORIENTED_GATHERER_VISITOR__

#include "galerkin/processing/visitors/ClusterLeafVisitor.h"
class OrientedGathererVisitor: public ClusterLeafVisitor{ private:
    Interaction *link;
    ColorRgbMutable *sourceRadiance;

  public:
    OrientedGathererVisitor(Interaction *inLink, ColorRgbMutable *inSourceRadiance);
    ~OrientedGathererVisitor();

    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState);
};

#endif
