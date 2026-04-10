/**
Some utility routines for ray intersections and for statistics
*/

#ifndef __RAY_TOOLS__
#define __RAY_TOOLS__

#include "common/linealAlgebra/Ray.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "skin/Patch.h"
#include "scene/Camera.h"
#include "scene/VoxelGrid.h"
#include "raycasting/common/SimpleRaytracingPathNode.h"

class RayTools {
  public:
    static RayHit *
    findRayIntersection(
        const VoxelGrid *sceneWorldVoxelGrid,
        Ray *ray,
        Patch *patch,
        const PhongBidirScattDistFunc *currentBsdf,
        RayHit *hitStore);

    static bool
    pathNodesVisible(
        const VoxelGrid *sceneWorldVoxelGrid,
        const SimpleRaytracingPathNode *node1,
        const SimpleRaytracingPathNode *node2);

    static bool
    eyeNodeVisible(
        const Camera *camera,
        const VoxelGrid *sceneWorldVoxelGrid,
        const SimpleRaytracingPathNode *eyeNode,
        const SimpleRaytracingPathNode *node,
        float *pixX,
        float *pixY);

  private:
    static int pathFrontHitFlags();

    static RayHit *
    traceWorld(
        const VoxelGrid *sceneWorldVoxelGrid,
        Ray *ray,
        Patch *patch,
        unsigned int flags,
        Patch *extraPatch,
        RayHit *hitStore);
};

#endif
