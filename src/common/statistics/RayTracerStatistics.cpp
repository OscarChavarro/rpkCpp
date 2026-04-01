#include "common/statistics/RayTracerStatistics.h"

RayTracerStatistics::RayTracerStatistics():
    currentRayTracer(nullptr),
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
