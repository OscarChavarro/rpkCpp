#ifndef __SHADOW_CACHING__
#define __SHADOW_CACHING__

#include "common/Ray.h"
#include "scene/VoxelGrid.h"

extern void initShadowCache();
extern RayHit *cacheHit(Ray *ray, float *dist, RayHit *hitStore);
extern void addToShadowCache(Patch *patch);
extern RayHit *
shadowTestDiscretization(
    Ray *ray,
    java::ArrayList<Geometry *> *geometrySceneList,
    VoxelGrid *voxelGrid,
    float dist,
    RayHit *hitStore,
    bool isSceneGeometry,
    bool isClusteredGeometry);

#endif