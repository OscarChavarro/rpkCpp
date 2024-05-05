/**
Some utility routines for ray intersections and for statistics
*/

#ifndef __RAY_TOOLS__
#define __RAY_TOOLS__

#include "common/Ray.h"
#include "material/BidirectionalScatteringDistributionFunction.h"
#include "skin/Patch.h"
#include "scene/Camera.h"
#include "scene/VoxelGrid.h"
#include "raycasting/common/pathnode.h"

extern RayHit *findRayIntersection(VoxelGrid *sceneWorldVoxelGrid, Ray *ray, Patch *patch, BidirectionalScatteringDistributionFunction *currentBsdf, RayHit *hitStore);

extern bool
pathNodesVisible(
    VoxelGrid *sceneWorldVoxelGrid,
    SimpleRaytracingPathNode *node1,
    SimpleRaytracingPathNode *node2);

extern bool
eyeNodeVisible(
    Camera *camera,
    VoxelGrid *sceneWorldVoxelGrid,
    SimpleRaytracingPathNode *eyeNode,
    SimpleRaytracingPathNode *node,
    float *pixX,
    float *pixY);

#endif
