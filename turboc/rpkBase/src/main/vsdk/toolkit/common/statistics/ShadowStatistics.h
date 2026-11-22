#ifndef __SHADOW_STATISTICS__
#define __SHADOW_STATISTICS__

#include "vsdk/toolkit/common/VSDK.h"

class ShadowStatistics {
  public:
    int numberOfShadowRays;
    int numberOfShadowCacheHits;

    ShadowStatistics();
    void reset();
};

#endif
