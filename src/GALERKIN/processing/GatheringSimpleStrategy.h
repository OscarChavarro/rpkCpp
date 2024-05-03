#ifndef __GATHERING_SIMPLE_STRATEGY__
#define __GATHERING_SIMPLE_STRATEGY__

#include "GALERKIN/processing/GatheringStrategy.h"

class GatheringSimpleStrategy final: public GatheringStrategy {
  private:
    static void patchUpdatePotential(Patch *patch);
    static void patchUpdateRadiance(Patch *patch, GalerkinState *galerkinState);

    static void
    patchLazyCreateInteractions(
        VoxelGrid *sceneWorldVoxelGrid,
        Patch *patch,
        GalerkinState *galerkinState,
        java::ArrayList<Geometry *> *sceneGeometries,
        java::ArrayList<Geometry *> *sceneClusteredGeometries);

    static void
    patchGather(
        Patch *patch,
        Scene *scene,
        GalerkinState *galerkinState,
        RenderOptions *renderOptions);

  public:
    GatheringSimpleStrategy();
    ~GatheringSimpleStrategy() final;

    bool doGatheringIteration(Scene *scene, GalerkinState *galerkinState, RenderOptions *renderOptions) final;
};

#endif
