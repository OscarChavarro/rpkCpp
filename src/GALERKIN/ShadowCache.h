#ifndef __SHADOW_CACHE__
#define __SHADOW_CACHE__

#include "skin/Patch.h"

// Cache blocking patches
#define MAX_CACHE 5

class ShadowCache {
  private:
    Patch *patchCache[MAX_CACHE];
    int numberOfCachedPatches;
  public:
    ShadowCache();
    virtual ~ShadowCache();

    RayHit *cacheHit(const Ray *ray, float *distance, RayHit *hitStore) const;

    void addToShadowCache(Patch *patch);
};

#endif
