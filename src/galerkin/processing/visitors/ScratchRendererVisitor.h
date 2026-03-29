#ifndef __SCRATCH_RENDERER_VISITOR__
#define __SCRATCH_RENDERER_VISITOR__

#include "galerkin/GalerkinElement.h"
#include "galerkin/processing/visitors/ClusterLeafVisitor.h"
class SglContext;

class ScratchRendererVisitor final: public ClusterLeafVisitor {
  private:
    Vector3D eyePoint;
    SglContext *sglContext;

  public:
    ScratchRendererVisitor(Vector3D inEyePoint, SglContext *inSglContext);
    ~ScratchRendererVisitor() final;
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState) final;
};

#endif
