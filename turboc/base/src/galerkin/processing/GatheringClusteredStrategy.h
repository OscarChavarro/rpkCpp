#ifndef GTHRN_CLSTR_STRTG
#define GTHRN_CLSTR_STRTG

#include "galerkin/processing/GatheringStrategy.h"

class GatheringClusteredStrategy: public GatheringStrategy{ private:
    static float updatePotential(GalerkinElement *cluster);
    static void updateClusterDirectPotential(GalerkinElement *element, float potential_increment);
  public:
    GatheringClusteredStrategy();
    ~GatheringClusteredStrategy();

    bool doGatheringIteration(const Scene *scene, GalerkinState *galerkinState, RenderOptions *renderOptions);
};

#endif
