#ifndef RADIANCE_STATISTICS__
#define RADIANCE_STATISTICS__

#include "vsdk/toolkit/common/color/ColorRgbMutable.h"

class RadianceStatistics {
  public:
    float totalArea;
    ColorRgbMutable maxSelfEmittedRadiance;
    ColorRgbMutable maxSelfEmittedPower;
    double referenceLuminance;
    ColorRgbMutable totalEmittedPower;
    ColorRgbMutable estimatedAverageRadiance;
    ColorRgbMutable averageReflectivity;

    RadianceStatistics();
    void reset();
};

#endif
