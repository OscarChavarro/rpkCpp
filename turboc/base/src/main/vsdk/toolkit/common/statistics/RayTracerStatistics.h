#ifndef __RAY_TRACER_STATISTICS__
#define __RAY_TRACER_STATISTICS__

#include "vsdk/toolkit/common/VSDK.h"

class RayTracerStatistics {
  public:
    double totalTime;
    long rayCount;
    long pixelCount;

    RayTracerStatistics();
    void resetCounters();
};

#endif
