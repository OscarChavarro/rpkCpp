#include "common/statistics/RadianceStatistics.h"

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
    maxSelfEmittedRadiance = ColorRgb(0.0f, 0.0f, 0.0f);
    maxSelfEmittedPower = ColorRgb(0.0f, 0.0f, 0.0f);
    referenceLuminance = 0.0;
    totalEmittedPower = ColorRgb(0.0f, 0.0f, 0.0f);
    estimatedAverageRadiance = ColorRgb(0.0f, 0.0f, 0.0f);
    averageReflectivity = ColorRgb(0.0f, 0.0f, 0.0f);
}
