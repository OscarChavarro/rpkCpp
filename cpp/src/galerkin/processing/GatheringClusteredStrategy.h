#ifndef __GATHERING_CLUSTERED_STRATEGY__
#define __GATHERING_CLUSTERED_STRATEGY__

#include "galerkin/processing/GatheringStrategy.h"

class GatheringClusteredStrategy final : public GatheringStrategy {
  private:
    static float updatePotential(GalerkinElement *cluster);
    static void updateClusterDirectPotential(GalerkinElement *element, float potential_increment);
  public:
    GatheringClusteredStrategy();
    ~GatheringClusteredStrategy() final;

    bool doGatheringIteration(const Scene *scene, GalerkinState *galerkinState, RendererConfiguration *renderOptions) final;
};

#endif
