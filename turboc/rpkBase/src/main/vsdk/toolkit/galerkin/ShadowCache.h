#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __SHADOW_CACHE__
#define __SHADOW_CACHE__

#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class ShadowCache {
  private:
    #define MAX_CACHE 5
    const Patch *patchCache[MAX_CACHE];
    int numberOfCachedPatches;
  public:
    ShadowCache();
    virtual ~ShadowCache();

    RpkRayHit *cacheHit(const Ray *ray, double *distance, RpkRayHit *hitStore, const RayExclusionSet &exclusions) const;
    void addToShadowCache(const Patch *patch);
};

#endif
