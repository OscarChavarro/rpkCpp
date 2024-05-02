#include "GALERKIN/ShadowCache.h"

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
RayHit *
ShadowCache::cacheHit(const Ray *ray, float *distance, RayHit *hitStore) const {
    for ( int i = 0; i < numberOfCachedPatches; i++ ) {
        RayHit *hit = patchCache[i]->intersect(
            ray,
            EPSILON_FLOAT *(*distance),
            distance,
            HIT_FRONT | HIT_ANY,
            hitStore);
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
ShadowCache::addToShadowCache(Patch *patch) {
    patchCache[numberOfCachedPatches % MAX_CACHE] = patch;
    if ( numberOfCachedPatches < MAX_CACHE ) {
        numberOfCachedPatches++;
    }
}
