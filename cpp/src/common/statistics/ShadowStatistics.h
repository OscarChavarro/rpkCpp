#ifndef __SHADOW_STATISTICS__
#define __SHADOW_STATISTICS__

class ShadowStatistics {
  public:
    int numberOfShadowRays;
    int numberOfShadowCacheHits;

    ShadowStatistics();
    void reset();
};

#endif
