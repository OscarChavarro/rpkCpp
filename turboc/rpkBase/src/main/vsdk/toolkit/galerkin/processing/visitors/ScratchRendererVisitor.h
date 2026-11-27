#ifndef __SCRATCH_RENDERER_VISITOR__
#define __SCRATCH_RENDERER_VISITOR__

#include "vsdk/toolkit/galerkin/GalerkinElement.h"
#include "vsdk/toolkit/galerkin/processing/visitors/ClusterLeafVisitor.h"
#include "vsdk/toolkit/galerkin/processing/ScratchVisibilityWorkspace.h"

class ScratchRendererVisitor: public ClusterLeafVisitor{ private:
    Vector3Dd eyePoint;
    ScratchVisibilityBuffer &target;

  public:
    ScratchRendererVisitor(Vector3Dd inEyePoint, ScratchVisibilityBuffer &inTarget);
    ~ScratchRendererVisitor();
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState);
};

#endif
