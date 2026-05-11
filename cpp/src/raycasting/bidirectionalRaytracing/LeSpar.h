#ifndef LE_SPAR__
#define LE_SPAR__

#include "raycasting/bidirectionalRaytracing/Spar.h"

/**
Le Spar : Uses emission ase stored radiance. Allows sampling of
all bidirectional paths
*/
class LeSpar final : public Spar {
  public:
    void init(SparConfig *sparConfig, RadianceMethod *radianceMethod) final;
};

#endif
