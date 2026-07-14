/**
Some utility routines for ray intersections and for statistics
*/

#ifndef RAY_TOOLS__
#define RAY_TOOLS__

#include "vsdk/toolkit/common/linealAlgebra/Ray.h"
#include "vsdk/toolkit/material/PhongBidirectionalScatteringDistributionFunction.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/scene/VoxelGrid.h"
#include "vsdk/toolkit/raycasting/common/SimpleRaytracingPathNode.h"

class RayTools {
  public:
    static RayHit *
    findRayIntersection(
        const VoxelGrid *sceneWorldVoxelGrid,
        Ray *ray,
        Patch *patch,
        const PhongBidirectionalScatteringDistributionFunction *currentBsdf,
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
