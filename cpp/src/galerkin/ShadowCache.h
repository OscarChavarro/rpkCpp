#ifndef SHADOW_CACHE__
#define SHADOW_CACHE__

#include "environment/geometry/elements/Patch.h"

class ShadowCache {
  private:
    static constexpr int MAX_CACHE = 5;
    Patch *patchCache[MAX_CACHE];
    int numberOfCachedPatches;
  public:
    ShadowCache();
    virtual ~ShadowCache();

    RayHit *cacheHit(const Ray *ray, float *distance, RayHit *hitStore) const;
    void addToShadowCache(Patch *patch);
};

#endif
