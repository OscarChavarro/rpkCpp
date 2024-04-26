#ifndef __GATHERING_STRATEGY__
#define __GATHERING_STRATEGY__

#include "scene/Scene.h"
#include "GALERKIN/GalerkinState.h"

/**
Numerical integration for Jacobi or Gauss-Seidel Galerkin radiosity.
*/
class GatheringStrategy {
  protected:
    static float pushPullPotential(GalerkinElement *element, float down);

  public:
    GatheringStrategy();
    virtual ~GatheringStrategy();

    virtual bool doGatheringIteration(Scene *scene, GalerkinState *galerkinState, RenderOptions *renderOptions) = 0;
    static void recomputePatchColor(Patch *patch, GalerkinState *galerkinState);
};

#endif
