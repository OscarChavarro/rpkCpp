#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __SHADOW_CACHE__
#define __SHADOW_CACHE__

#include "skin/Patch.h"

class ShadowCache {
  private:
    #define MAX_CACHE 5
    Patch *patchCache[MAX_CACHE];
    int numberOfCachedPatches;
  public:
    ShadowCache();
    virtual ~ShadowCache();

    RayHit *cacheHit(const Ray *ray, float *distance, RayHit *hitStore) const;
    void addToShadowCache(Patch *patch);
};

#endif
