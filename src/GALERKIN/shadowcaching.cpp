#include "material/statistics.h"
#include "GALERKIN/shadowcaching.h"

// Cache at most 5 blocking patches
#define MAX_CACHE 5

static Patch *globalCache[MAX_CACHE];
static int globalCachedPatches;
static int globalNumberOfCachedPatches;

/**
Initialize/empty the shadow cache
*/
void
initShadowCache() {
    globalNumberOfCachedPatches = globalCachedPatches = 0;
    for ( int i = 0; i < MAX_CACHE; i++ ) {
        globalCache[i] = nullptr;
    }
}

/**
Test ray against patches in the shadow cache. Returns nullptr if the ray hits
no patches in the shadow cache, or a pointer to the first hit patch otherwise
*/
RayHit *
cacheHit(Ray *ray, float *dist, RayHit *hitStore) {
    RayHit *hit;

    for ( int i = 0; i < globalNumberOfCachedPatches; i++ ) {
        hit = globalCache[i]->intersect(ray, EPSILON * (*dist), dist, HIT_FRONT | HIT_ANY, hitStore);
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
addToShadowCache(Patch *patch) {
    globalCache[globalCachedPatches % MAX_CACHE] = patch;
    globalCachedPatches++;
    if ( globalNumberOfCachedPatches < MAX_CACHE ) {
        globalNumberOfCachedPatches++;
    }
}

/**
Tests whether the ray intersects the discretization of a geometry in the list
'world'. Returns nullptr if the ray hits no geometries. Returns an arbitrary hit
patch if the ray does intersect one or more geometries. Intersections
further away than distance are ignored.
*/
RayHit *
shadowTestDiscretization(
    Ray *ray,
    java::ArrayList<Geometry *> *geometrySceneList,
    VoxelGrid *voxelGrid,
    float dist,
    RayHit *hitStore,
    bool isSceneGeometry,
    bool isClusteredGeometry)
{
    RayHit *hit;

    GLOBAL_statistics.numberOfShadowRays++;
    hit = cacheHit(ray, &dist, hitStore);
    if ( hit != nullptr ) {
        GLOBAL_statistics.numberOfShadowCacheHits++;
    } else {
        if ( !isClusteredGeometry && !isSceneGeometry ) {
            hit = geometryListDiscretizationIntersect(geometrySceneList, ray, EPSILON * dist, &dist,
                                                      HIT_FRONT | HIT_ANY, hitStore);
        } else {
            hit = voxelGrid->gridIntersect(ray, EPSILON * dist, &dist, HIT_FRONT | HIT_ANY, hitStore);
        }
        if ( hit ) {
            addToShadowCache(hit->patch);
        }
    }

    return hit;
}
