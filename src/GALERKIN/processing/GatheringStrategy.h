#ifndef __GATHERING_STRATEGY__
#define __GATHERING_STRATEGY__

#include "scene/Scene.h"
#include "GALERKIN/GalerkinState.h"

/**
Reference:
[COHE1993] Cohen, M. Wallace, J. "Radiosity and Realistic Image Synthesis",
     Academic Press Professional, 1993.
*/

/**
Numerical integration for Jacobi or Gauss-Seidel Galerkin radiosity.
See [COHE1993] section 5.3.2.
*/
class GatheringStrategy {
  protected:
    static float pushPullPotential(GalerkinElement *element, float down);

  public:
    GatheringStrategy();
    virtual ~GatheringStrategy();

    virtual bool doGatheringIteration(const Scene *scene, GalerkinState *galerkinState, RenderOptions *renderOptions) = 0;
    static void recomputePatchColor(Patch *patch, GalerkinState *galerkinState);
};

#endif
