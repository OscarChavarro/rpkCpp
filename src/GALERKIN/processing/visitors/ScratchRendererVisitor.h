#ifndef __SCRATCH_RENDERER_VISITOR__
#define __SCRATCH_RENDERER_VISITOR__

#include "GALERKIN/processing/visitors/ClusterLeafVisitor.h"
#include "GALERKIN/GalerkinElement.h"

class ScratchRendererVisitor final: public ClusterLeafVisitor {
  private:
    Vector3D eyePoint;

  public:
    explicit ScratchRendererVisitor(Vector3D inEyePoint);
    ~ScratchRendererVisitor() final;
    void visit(GalerkinElement *elem, const GalerkinState *galerkinState, ColorRgb *accumulatedRadiance) const final;
};

#endif
