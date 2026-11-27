#ifndef __REFINEMENT_WORKSPACE__
#define __REFINEMENT_WORKSPACE__

#include "vsdk/toolkit/common/memoryManagement/MemoryPool.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/galerkin/processing/FormFactorWorkspace.h"

class ScratchVisibilityWorkspace;

/**
Mutable scratch of hierarchical refinement: the scene's query workspace, the
solver's form factor and scratch visibility workspaces and the pool of the
coupling coefficients of the sub-interactions. Nothing is owned.

Serial Turbo C++ port: the cpp port's workspace also carries, for its parallel
Jacobi workers, a record of what is to be committed later in the serial order
(RefinementRecord, RefinementEvent), the lock shared for regular subdivision
and the task scheduler (RefinementTask, RefinementScheduler); they are null
for the cpp serial solver and are left out here, together with the code that
uses them.
*/
class RefinementWorkspace {
  public:
    RefinementWorkspace(
        SceneQueryWorkspace *inSceneQueryWorkspace,
        FormFactorWorkspace *inFormFactorWorkspace,
        ScratchVisibilityWorkspace *inScratchVisibilityWorkspace,
        MemoryPool<double> *inCoefficientsPool):
        sceneQueryWorkspace(inSceneQueryWorkspace),
        formFactorWorkspace(inFormFactorWorkspace),
        scratchVisibilityWorkspace(inScratchVisibilityWorkspace),
        coefficientsPool(inCoefficientsPool) {}

    SceneQueryWorkspace *getSceneQueryWorkspace() const { return sceneQueryWorkspace; }
    FormFactorWorkspace *getFormFactorWorkspace() const { return formFactorWorkspace; }

    /** NULL unless the clustering strategy is Z-visibility */
    ScratchVisibilityWorkspace *getScratchVisibilityWorkspace() const { return scratchVisibilityWorkspace; }
    MemoryPool<double> *getCoefficientsPool() const { return coefficientsPool; }

  private:
    SceneQueryWorkspace *sceneQueryWorkspace;
    FormFactorWorkspace *formFactorWorkspace;
    ScratchVisibilityWorkspace *scratchVisibilityWorkspace;
    MemoryPool<double> *coefficientsPool;
};

#endif
