#ifndef __SCRATCH_RENDERER_VISITOR__
#define __SCRATCH_RENDERER_VISITOR__

#include "GALERKIN/GalerkinElement.h"
#include "GALERKIN/processing/visitors/ClusterLeafVisitor.h"

class ScratchRendererVisitor final: public ClusterLeafVisitor {
  private:
    Vector3D eyePoint;

  public:
    explicit ScratchRendererVisitor(Vector3D inEyePoint);
    ~ScratchRendererVisitor() final;
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState, ColorRgb *accumulatedRadiance) final;
};

#endif
