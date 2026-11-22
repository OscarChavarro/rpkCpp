#ifndef SHADOW_CACHE__
#define SHADOW_CACHE__

#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class ShadowCache {
  private:
    static constexpr int MAX_CACHE = 5;
    const Patch *patchCache[MAX_CACHE];
    int numberOfCachedPatches;
  public:
    ShadowCache();
    virtual ~ShadowCache();

    RpkRayHit *cacheHit(const RpkRay *ray, float *distance, RpkRayHit *hitStore, const RayExclusionSet &exclusions) const;
    void addToShadowCache(const Patch *patch);
};

#endif
