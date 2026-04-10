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
    maxSelfEmittedRadiance.clear();
    maxSelfEmittedPower.clear();
    referenceLuminance = 0.0;
    totalEmittedPower.clear();
    estimatedAverageRadiance.clear();
    averageReflectivity.clear();
}
