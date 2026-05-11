#ifndef DEPTH_VISIBILITY_GATHERER_VISITOR__
#define DEPTH_VISIBILITY_GATHERER_VISITOR__

#include "galerkin/processing/visitors/ClusterLeafVisitor.h"
class DepthVisibilityGathererVisitor final : public ClusterLeafVisitor {
  private:
    Interaction *link;
    ColorRgb *sourceRadiance;
    double pixelArea;

  public:
    DepthVisibilityGathererVisitor(Interaction *inLink, ColorRgb *inSourceRadiance, double inPixelArea);
    ~DepthVisibilityGathererVisitor() final;
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState) final;
};

#endif
