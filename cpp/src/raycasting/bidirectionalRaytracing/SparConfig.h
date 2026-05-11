#ifndef SPAR_CONFIG__
#define SPAR_CONFIG__

#include "raycasting/bidirectionalRaytracing/BidirectionalPathRaytracerConfig.h"
class Spar;

// Spar Config stores handy config params
class SparConfig {
  public:
    BidirectionalPathRaytracerConfig *baseConfig;

    // Needed in weighted multi-pass methods
    Spar *leSpar;
    Spar *ldSpar;
};

#endif
