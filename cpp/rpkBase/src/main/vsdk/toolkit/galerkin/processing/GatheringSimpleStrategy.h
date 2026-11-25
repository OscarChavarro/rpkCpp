#ifndef __GATHERING_SIMPLE_STRATEGY__
#define __GATHERING_SIMPLE_STRATEGY__

#include "vsdk/toolkit/galerkin/processing/GatheringStrategy.h"

class GatheringSimpleStrategy final: public GatheringStrategy {
  private:
    static void patchUpdatePotential(const Patch *patch, const DirectPotentialTable *directPotentials, const GalerkinState *galerkinState);
    static void patchUpdateRadiance(Patch *patch, GalerkinState *galerkinState, FaceColorTable *faceColors);

    static void
    patchLazyCreateInteractions(
        const Scene *scene,
        const Patch *patch,
        const GalerkinState *galerkinState);

    static void
    patchGather(
        Patch *patch,
        const Scene *scene,
        GalerkinState *galerkinState);

  public:
    GatheringSimpleStrategy();
    ~GatheringSimpleStrategy() final;

    bool doGatheringIteration(const Scene *scene, GalerkinState *galerkinState, RendererConfiguration *renderOptions) final;
};

#endif
