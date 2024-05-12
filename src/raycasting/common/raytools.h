/**
Some utility routines for ray intersections and for statistics
*/

#ifndef __RAY_TOOLS__
#define __RAY_TOOLS__

#include "common/Ray.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "skin/Patch.h"
#include "scene/Camera.h"
#include "scene/VoxelGrid.h"
#include "raycasting/common/pathnode.h"

extern RayHit *
findRayIntersection(
    const VoxelGrid *sceneWorldVoxelGrid,
    Ray *ray,
    Patch *patch,
    const PhongBidirectionalScatteringDistributionFunction *currentBsdf,
    RayHit *hitStore);

extern bool
pathNodesVisible(
    const VoxelGrid *sceneWorldVoxelGrid,
    const SimpleRaytracingPathNode *node1,
    const SimpleRaytracingPathNode *node2);

extern bool
eyeNodeVisible(
    const Camera *camera,
    const VoxelGrid *sceneWorldVoxelGrid,
    const SimpleRaytracingPathNode *eyeNode,
    const SimpleRaytracingPathNode *node,
    float *pixX,
    float *pixY);

#endif
