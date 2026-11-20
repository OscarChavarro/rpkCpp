/**
Some utility routines for ray intersections and for statistics
*/

#ifndef RAY_TOOLS__
#define RAY_TOOLS__

#include "vsdk/toolkit/common/linealAlgebra/RpkRay.h"
#include "vsdk/toolkit/material/PhongBidirectionalScatteringDistributionFunction.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/scene/VoxelGrid.h"
#include "vsdk/toolkit/raycasting/common/SimpleRaytracingPathNode.h"

class RayTools {
  public:
    static RpkRayHit *
    findRayIntersection(
        const VoxelGrid *sceneWorldVoxelGrid,
        VoxelGridWorkspace *sceneWorldVoxelGridWorkspace,
        RpkRay *ray,
        const Patch *patch,
        const PhongBidirectionalScatteringDistributionFunction *currentBsdf,
        RpkRayHit *hitStore);

    static bool
    pathNodesVisible(
        const VoxelGrid *sceneWorldVoxelGrid,
        VoxelGridWorkspace *sceneWorldVoxelGridWorkspace,
        const SimpleRaytracingPathNode *node1,
        const SimpleRaytracingPathNode *node2);

    static bool
    eyeNodeVisible(
        const Camera *camera,
        const VoxelGrid *sceneWorldVoxelGrid,
        VoxelGridWorkspace *sceneWorldVoxelGridWorkspace,
        const SimpleRaytracingPathNode *eyeNode,
        const SimpleRaytracingPathNode *node,
        float *pixX,
        float *pixY);

  private:
    static int pathFrontHitFlags();

    static RpkRayHit *
    traceWorld(
        const VoxelGrid *sceneWorldVoxelGrid,
        VoxelGridWorkspace *sceneWorldVoxelGridWorkspace,
        RpkRay *ray,
        const Patch *patch,
        unsigned int flags,
        const Patch *extraPatch,
        RpkRayHit *hitStore);
};

#endif
