#ifndef __RAY_MATTER_STATE__
#define __RAY_MATTER_STATE__

#include "raycasting/simple/RayMatterFilterType.h"

class RayMatterState {
  public:
    int samplesPerPixel; // Pixel sampling
    RayMatterFilterType filter; // Pixel filter
};

#endif
