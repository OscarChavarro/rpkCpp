#include "vsdk/toolkit/common/statistics/RayTracerStatistics.h"

RayTracerStatistics::RayTracerStatistics():
    totalTime(0.0),
    rayCount(0),
    pixelCount(0)
{
}

void
RayTracerStatistics::resetCounters() {
    totalTime = 0.0;
    rayCount = 0;
    pixelCount = 0;
}
