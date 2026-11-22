#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"
#include "vsdk/toolkit/galerkin/ShadowCache.h"

/**
Initialize/empty the shadow cache
*/
ShadowCache::ShadowCache(): patchCache() {
    numberOfCachedPatches = 0;
    for ( int i = 0; i < MAX_CACHE; i++ ) {
        patchCache[i] = nullptr;
    }
}

ShadowCache::~ShadowCache() {
    numberOfCachedPatches = 0;
}

/**
Test ray against patches in the shadow cache. Returns nullptr if the ray hits
no patches in the shadow cache, or a pointer to the first hit patch otherwise
*/
RpkRayHit *
ShadowCache::cacheHit(const RpkRay *ray, float *distance, RpkRayHit *hitStore, const RayExclusionSet &exclusions) const {
    for ( int i = 0; i < numberOfCachedPatches; i++ ) {
        RpkRayHit *hit = patchCache[i]->intersect(
            ray,
            Numeric::EPSILON_FLOAT *(*distance),
            distance,
            RayHitFlag::FRONT | RayHitFlag::ANY,
            hitStore,
            exclusions);
        if ( hit != nullptr ) {
            return hit;
        }
    }
    return nullptr;
}

/**
Replace least recently added patch
*/
void
ShadowCache::addToShadowCache(const Patch *patch) {
    patchCache[numberOfCachedPatches % MAX_CACHE] = patch;
    if ( numberOfCachedPatches < MAX_CACHE ) {
        numberOfCachedPatches++;
    }
}
