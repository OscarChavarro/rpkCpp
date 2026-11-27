#include "vsdk/toolkit/common/statistics/RadianceStatistics.h"

RadianceStatistics::RadianceStatistics():
    totalArea(0.0),
    maxSelfEmittedRadiance(),
    maxSelfEmittedPower(),
    referenceLuminance(0.0),
    totalEmittedPower(),
    estimatedAverageRadiance(),
    averageReflectivity()
{
}

void
RadianceStatistics::reset() {
    totalArea = 0.0;
    maxSelfEmittedRadiance = ColorRgb(0.0, 0.0, 0.0);
    maxSelfEmittedPower = ColorRgb(0.0, 0.0, 0.0);
    referenceLuminance = 0.0;
    totalEmittedPower = ColorRgb(0.0, 0.0, 0.0);
    estimatedAverageRadiance = ColorRgb(0.0, 0.0, 0.0);
    averageReflectivity = ColorRgb(0.0, 0.0, 0.0);
}
