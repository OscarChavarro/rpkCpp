#ifndef __SCRATCH_RENDERER_VISITOR__
#define __SCRATCH_RENDERER_VISITOR__

#include "galerkin/GalerkinElement.h"
#include "galerkin/processing/visitors/ClusterLeafVisitor.h"
#include "render/sgl/SglContext.h"

class ScratchRendererVisitor: public ClusterLeafVisitor{ private:
    Vector3D eyePoint;
    SglContext *sglContext;

  public:
    ScratchRendererVisitor(Vector3D inEyePoint, SglContext *inSglContext);
    ~ScratchRendererVisitor();
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState);
};

#endif
