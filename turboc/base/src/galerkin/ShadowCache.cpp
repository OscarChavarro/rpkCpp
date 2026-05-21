#include "environment/geometry/elements/RayHitFlag.h"
#include "galerkin/ShadowCache.h"

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
RayHit *
ShadowCache::cacheHit(const Ray *ray, float *distance, RayHit *hitStore) const {
    for ( int i = 0; i < numberOfCachedPatches; i++ ) {
        RayHit *hit = patchCache[i]->intersect(
            ray,
            Numeric::EPSILON_FLOAT *(*distance),
            distance,
            FRONT | ANY,
            hitStore);
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
ShadowCache::addToShadowCache(Patch *patch) {
    patchCache[numberOfCachedPatches % MAX_CACHE] = patch;
    if ( numberOfCachedPatches < MAX_CACHE ) {
        numberOfCachedPatches++;
    }
}
