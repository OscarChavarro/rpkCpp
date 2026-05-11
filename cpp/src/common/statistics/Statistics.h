#ifndef STATISTICS__
#define STATISTICS__

#include "common/statistics/ReaderStatistics.h"
#include "common/statistics/RadianceStatistics.h"
#include "common/statistics/PotentialStatistics.h"
#include "common/statistics/ShadowStatistics.h"
#include "common/statistics/RayTracerStatistics.h"

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
