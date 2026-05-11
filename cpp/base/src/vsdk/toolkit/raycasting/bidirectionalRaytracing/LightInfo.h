#ifndef LIGHT_INFO__
#define LIGHT_INFO__

#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class LightInfo {
  public:
    float emittedFlux;
    float importance; // Cumulative probability : for importance sampling
    Patch *light;
};

#endif
