#ifndef RADIANCE_STATISTICS__
#define RADIANCE_STATISTICS__

#include "vsdk/toolkit/common/color/ColorRgb.h"

class RadianceStatistics {
  public:
    float totalArea;
    ColorRgb maxSelfEmittedRadiance;
    ColorRgb maxSelfEmittedPower;
    double referenceLuminance;
    ColorRgb totalEmittedPower;
    ColorRgb estimatedAverageRadiance;
    ColorRgb averageReflectivity;

    RadianceStatistics();
    void reset();
};

#endif
