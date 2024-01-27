#ifndef __SHADOW_CACHING__
#define __SHADOW_CACHING__

#include "skin/PatchSet.h"
#include "skin/geomlist.h"
#include "common/Ray.h"

extern void initShadowCache();
extern RayHit *cacheHit(Ray *ray, float *dist, RayHit *hitStore);
extern void addToShadowCache(Patch *patch);
extern RayHit *shadowTestDiscretization(Ray *ray, GeometryListNode *world, float dist, RayHit *hitStore);

#endif
