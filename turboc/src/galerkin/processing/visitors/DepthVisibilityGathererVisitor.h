#ifndef DPTH_VSBLT_GTHRR_VSTR
#define DPTH_VSBLT_GTHRR_VSTR

#include "galerkin/processing/visitors/ClusterLeafVisitor.h"
class DepthVisibilityGathererVisitor: public ClusterLeafVisitor{ private:
    Interaction *link;
    ColorRgb *sourceRadiance;
    double pixelArea;

  public:
    DepthVisibilityGathererVisitor(Interaction *inLink, ColorRgb *inSourceRadiance, double inPixelArea);
    ~DepthVisibilityGathererVisitor();
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState);
};

#endif
