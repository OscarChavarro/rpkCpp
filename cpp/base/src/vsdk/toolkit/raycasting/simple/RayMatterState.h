#ifndef RAY_MATTER_STATE__
#define RAY_MATTER_STATE__

#include "vsdk/toolkit/raycasting/simple/RayMatterFilterType.h"

class RayMatterState {
  public:
    RayMatterState():
        samplesPerPixel(8),
        filter(TENT_FILTER)
    {
    }

    int samplesPerPixel; // Pixel sampling
    RayMatterFilterType filter; // Pixel filter
};

#endif
