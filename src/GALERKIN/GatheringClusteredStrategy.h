#ifndef __GATHERING_CLUSTERED_STRATEGY__
#define __GATHERING_CLUSTERED_STRATEGY__

#include "GALERKIN/GatheringStrategy.h"

class GatheringClusteredStrategy : public GatheringStrategy {
  private:
    static float updatePotential(GalerkinElement *cluster);
    static void updateClusterDirectPotential(GalerkinElement *element, float potential_increment);
  public:
    GatheringClusteredStrategy();
    virtual ~GatheringClusteredStrategy();

    bool doGatheringIteration(Scene *scene, GalerkinState *galerkinState, RenderOptions *renderOptions);
};

#endif
