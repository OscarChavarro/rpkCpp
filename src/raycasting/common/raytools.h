/**
Some utility routines for ray intersections and for statistics
*/

#ifndef __RAY_TOOLS__
#define __RAY_TOOLS__

#include "common/Ray.h"
#include "material/bsdf.h"
#include "skin/Patch.h"
#include "raycasting/common/pathnode.h"

extern RayHit *findRayIntersection(VoxelGrid *sceneWorldVoxelGrid, Ray *ray, Patch *patch, BSDF *currentBsdf, RayHit *hitStore);

extern bool
pathNodesVisible(
    VoxelGrid *sceneWorldVoxelGrid,
    SimpleRaytracingPathNode *node1,
    SimpleRaytracingPathNode *node2);

extern bool
eyeNodeVisible(
    VoxelGrid *sceneWorldVoxelGrid,
    SimpleRaytracingPathNode *eyeNode,
    SimpleRaytracingPathNode *node,
    float *pixX,
    float *pixY);

#endif
