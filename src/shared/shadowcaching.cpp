#include "material/statistics.h"
#include "skin/Patch.h"
#include "scene/VoxelGrid.h"
#include "scene/scene.h"
#include "shared/shadowcaching.h"

#define MAXCACHE 5    /* cache at most 5 blocking patches */
static PATCH *cache[MAXCACHE];
static int cachedpatches, ncached;

/* initialize/empty the shadow cache */
void InitShadowCache() {
    int i;

    ncached = cachedpatches = 0;
    for ( i = 0; i < MAXCACHE; i++ ) {
        cache[i] = nullptr;
    }
}

/* test ray against patches in the shadow cache. Returns nullptr if the ray hits
 * no patches in the shadow cache, or a pointer to the first hit patch otherwise.  */
HITREC *CacheHit(Ray *ray, float *dist, HITREC *hitstore) {
    int i;
    HITREC *hit;

    for ( i = 0; i < ncached; i++ ) {
        if ((hit = patchIntersect(cache[i], ray, EPSILON * (*dist), dist, HIT_FRONT | HIT_ANY, hitstore))) {
            return hit;
        }
    }
    return nullptr;
}

/* replace least recently added patch */
void AddToShadowCache(PATCH *patch) {
    cache[cachedpatches % MAXCACHE] = patch;
    cachedpatches++;
    if ( ncached < MAXCACHE ) {
        ncached++;
    }
}

/* Tests whether the ray intersects the discretisation of a GEOMetry in the list 
 * 'world'. Returns nullptr if the ray hits no geometries. Returns an arbitrary hit 
 * patch if the ray does intersect one or more geometries. Intersections
 * further away than dist are ignored. GLOBAL_scene_patches in the shadow cache are
 * checked first. */
HITREC *ShadowTestDiscretisation(Ray *ray, GeometryListNode *world, float dist, HITREC *hitstore) {
    HITREC *hit = nullptr;

    GLOBAL_statistics_numberOfShadowRays++;
    if ((hit = CacheHit(ray, &dist, hitstore))) {
        GLOBAL_statistics_numberOfShadowCacheHits++;
    } else {
        if ( world != GLOBAL_scene_clusteredWorld && world != GLOBAL_scene_world ) {
            hit = GeomListDiscretizationIntersect(world, ray, EPSILON * dist, &dist, HIT_FRONT | HIT_ANY, hitstore);
        } else {
            hit = GLOBAL_scene_worldVoxelGrid->gridIntersect(ray, EPSILON * dist, &dist, HIT_FRONT | HIT_ANY, hitstore);
        }
        if ( hit ) {
            AddToShadowCache(hit->patch);
        }
    }

    return hit;
}
