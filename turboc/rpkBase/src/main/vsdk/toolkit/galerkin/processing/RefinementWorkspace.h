#ifndef __REFINEMENT_WORKSPACE__
#define __REFINEMENT_WORKSPACE__

#include "vsdk/toolkit/common/memoryManagement/MemoryPool.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/galerkin/processing/FormFactorWorkspace.h"

/**
Mutable scratch of hierarchical refinement: the scene's query workspace, the
solver's form factor workspace and the pool of the coupling coefficients of
the sub-interactions. Nothing is owned.

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
        MemoryPool<double> *inCoefficientsPool):
        sceneQueryWorkspace(inSceneQueryWorkspace),
        formFactorWorkspace(inFormFactorWorkspace),
        coefficientsPool(inCoefficientsPool) {}

    SceneQueryWorkspace *getSceneQueryWorkspace() const { return sceneQueryWorkspace; }
    FormFactorWorkspace *getFormFactorWorkspace() const { return formFactorWorkspace; }
    MemoryPool<double> *getCoefficientsPool() const { return coefficientsPool; }

  private:
    SceneQueryWorkspace *sceneQueryWorkspace;
    FormFactorWorkspace *formFactorWorkspace;
    MemoryPool<double> *coefficientsPool;
};

#endif
