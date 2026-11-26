#ifndef __LIGHT_INFO__
#define __LIGHT_INFO__

#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class LightInfo {
  public:
    double emittedFlux;
    double importance; // Cumulative probability : for importance sampling
    Patch *light;
};

#endif
