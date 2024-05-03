#ifndef __GATHERING_SIMPLE_STRATEGY__
#define __GATHERING_SIMPLE_STRATEGY__

#include "GALERKIN/processing/GatheringStrategy.h"

class GatheringSimpleStrategy final: public GatheringStrategy {
  private:
    static void patchUpdatePotential(const Patch *patch);
    static void patchUpdateRadiance(Patch *patch, GalerkinState *galerkinState);

    static void
    patchLazyCreateInteractions(
        const Scene *scene,
        const Patch *patch,
        const GalerkinState *galerkinState);

    static void
    patchGather(
        Patch *patch,
        const Scene *scene,
        GalerkinState *galerkinState,
        RenderOptions *renderOptions);

  public:
    GatheringSimpleStrategy();
    ~GatheringSimpleStrategy() final;

    bool doGatheringIteration(const Scene *scene, GalerkinState *galerkinState, RenderOptions *renderOptions) final;
};

#endif
