#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"
#include "vsdk/toolkit/galerkin/ShadowCache.h"

/**
Initialize/empty the shadow cache
*/
ShadowCache::ShadowCache(): patchCache() {
    numberOfCachedPatches = 0;
    for ( int i = 0; i < MAX_CACHE; i++ ) {
        patchCache[i] = NULL;
    }
}

ShadowCache::~ShadowCache() {
    numberOfCachedPatches = 0;
}

/**
Test ray against patches in the shadow cache. Returns NULL if the ray hits
no patches in the shadow cache, or a pointer to the first hit patch otherwise
*/
RpkRayHit *
ShadowCache::cacheHit(const Ray *ray, double *distance, RpkRayHit *hitStore, const RayExclusionSet &exclusions) const {
    for ( int i = 0; i < numberOfCachedPatches; i++ ) {
        RpkRayHit *hit = patchCache[i]->intersect(
            ray,
            Numeric::EPSILON *(*distance),
            distance,
            FRONT | ANY,
            hitStore,
            exclusions);
        if ( hit != NULL ) {
            return hit;
        }
    }
    return NULL;
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
