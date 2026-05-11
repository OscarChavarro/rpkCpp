#ifndef SHADOW_STATISTICS__
#define SHADOW_STATISTICS__

class ShadowStatistics {
  public:
    int numberOfShadowRays;
    int numberOfShadowCacheHits;

    ShadowStatistics();
    void reset();
};

#endif
