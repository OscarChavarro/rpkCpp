#ifndef SCRATCH_RENDERER_VISITOR__
#define SCRATCH_RENDERER_VISITOR__

#include "vsdk/toolkit/galerkin/GalerkinElement.h"
#include "vsdk/toolkit/galerkin/processing/visitors/ClusterLeafVisitor.h"
#include "vsdk/toolkit/render/sgl/SglContext.h"

class ScratchRendererVisitor final: public ClusterLeafVisitor {
  private:
    Vector3Df eyePoint;
    SglContext *sglContext;

  public:
    ScratchRendererVisitor(Vector3Df inEyePoint, SglContext *inSglContext);
    ~ScratchRendererVisitor() final;
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState) final;
};

#endif
