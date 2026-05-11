#ifndef LD_SPAR__
#define LD_SPAR__

#include "raycasting/bidirectionalRaytracing/Spar.h"

/**
LD Spar : Uses direct diffuse as stored radiance. Allows sampling of
of eye paths. GetDirectRadiance is used as a readout function
*/
class LDSpar final : public Spar {
  public:
    void init(SparConfig *sparConfig, RadianceMethod *radianceMethod) final;
};

#endif
