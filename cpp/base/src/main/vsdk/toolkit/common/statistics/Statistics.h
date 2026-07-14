#ifndef STATISTICS__
#define STATISTICS__

#include "vsdk/toolkit/common/statistics/ReaderStatistics.h"
#include "vsdk/toolkit/common/statistics/RadianceStatistics.h"
#include "vsdk/toolkit/common/statistics/PotentialStatistics.h"
#include "vsdk/toolkit/common/statistics/ShadowStatistics.h"
#include "vsdk/toolkit/common/statistics/RayTracerStatistics.h"

class Statistics {
  public:
    ReaderStatistics reader;
    RadianceStatistics radiance;
    PotentialStatistics potential;
    ShadowStatistics shadow;
    RayTracerStatistics rayTracer;

    Statistics();
    static Statistics &instance();
};

#endif
