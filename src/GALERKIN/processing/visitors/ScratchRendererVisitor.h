#ifndef __SCRATCH_RENDERER_VISITOR__
#define __SCRATCH_RENDERER_VISITOR__

#include "GALERKIN/GalerkinElement.h"
#include "GALERKIN/processing/visitors/ClusterLeafVisitor.h"

class SGL_CONTEXT;

class ScratchRendererVisitor final: public ClusterLeafVisitor {
  private:
    Vector3D eyePoint;
    SGL_CONTEXT *sglContext;

  public:
    ScratchRendererVisitor(Vector3D inEyePoint, SGL_CONTEXT *inSglContext);
    ~ScratchRendererVisitor() final;
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState) final;
};

#endif
