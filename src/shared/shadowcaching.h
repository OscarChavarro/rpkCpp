/* shadowcaching.h: shadow caching routines. */

#ifndef _SHADOWCACHING_H_
#define _SHADOWCACHING_H_

#include "skin/patchlist.h"
#include "skin/geomlist.h"
#include "common/Ray.h"

/* initialize/empty the shadow cache */
extern void InitShadowCache();

/* test ray against patches in the shadow cache. Returns nullptr if the ray hits
 * no patches in the shadow cache, or a pointer to the first hit patch 
 * otherwise.  */
extern HITREC *CacheHit(Ray *ray, float *dist, HITREC *hitstore);

/* replace least recently added patch */
extern void AddToShadowCache(PATCH *patch);

/* Tests whether the ray intersects the discretisation of a GEOMetry in the list 
 * 'world'. Returns nullptr if the ray hits no geometries. Returns an arbitrary hit 
 * patch if the ray does intersect one or more geometries. Intersections
 * further away than dist are ignored. GLOBAL_scene_patches in the shadow cache are
 * checked first. */
extern HITREC *ShadowTestDiscretisation(Ray *ray, GEOMLIST *world, float dist, HITREC *hitstore);

#endif
