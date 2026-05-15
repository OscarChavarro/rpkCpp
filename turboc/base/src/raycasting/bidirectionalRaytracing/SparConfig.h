#ifndef __SPAR_CONFIG__
#define __SPAR_CONFIG__

#include "common/VSDK.h"

#include "raycasting/bidirectionalRaytracing/BidirectionalPathRaytracerConfig.h"
class Spar;

// Spar Config stores handy config params
class SparConfig {
  public:
    BidirPathRaytrcCnfg *baseConfig;

    // Needed in weighted multi-pass methods
    Spar *leSpar;
    Spar *ldSpar;
};

#endif
