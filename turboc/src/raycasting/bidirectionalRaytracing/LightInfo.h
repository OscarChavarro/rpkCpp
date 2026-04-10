#ifndef __LIGHT_INFO__
#define __LIGHT_INFO__

#include "skin/Patch.h"

class LightInfo {
  public:
    float emittedFlux;
    float importance; // Cumulative probability : for importance sampling
    Patch *light;
};

#endif
