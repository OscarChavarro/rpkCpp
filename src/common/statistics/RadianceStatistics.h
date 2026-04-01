#ifndef __RADIANCE_STATISTICS__
#define __RADIANCE_STATISTICS__

#include "common/ColorRgb.h"

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
