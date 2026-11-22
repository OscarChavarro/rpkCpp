#ifndef __SCRATCH_RENDERER_VISITOR__
#define __SCRATCH_RENDERER_VISITOR__

#include "vsdk/toolkit/galerkin/GalerkinElement.h"
#include "vsdk/toolkit/galerkin/processing/visitors/ClusterLeafVisitor.h"
#include "vsdk/toolkit/render/sgl/SglContext.h"

class ScratchRendererVisitor: public ClusterLeafVisitor{ private:
    Vector3Df eyePoint;
    SglContext *sglContext;

  public:
    ScratchRendererVisitor(Vector3Df inEyePoint, SglContext *inSglContext);
    ~ScratchRendererVisitor();
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState);
};

#endif
