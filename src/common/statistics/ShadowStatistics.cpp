#include "common/statistics/ShadowStatistics.h"

ShadowStatistics::ShadowStatistics():
    numberOfShadowRays(0),
    numberOfShadowCacheHits(0)
{
}

void
ShadowStatistics::reset() {
    numberOfShadowRays = 0;
    numberOfShadowCacheHits = 0;
}
