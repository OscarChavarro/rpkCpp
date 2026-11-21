/**
Some utility routines for ray intersections and for statistics
*/

#ifndef __RAY_TOOLS__
#define __RAY_TOOLS__

#include "vsdk/toolkit/common/linealAlgebra/RpkRay.h"
#include "vsdk/toolkit/material/PhongBidirectionalScatteringDistributionFunction.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/raycasting/common/SimpleRaytracingPathNode.h"

class RayTools {
  public:
    static RayHit *
    findRayIntersection(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        RpkRay *ray,
        Patch *patch,
        const PhongBidirScattDistFunc *currentBsdf,
        RayHit *hitStore);

    static bool
    pathNodesVisible(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        const SimpleRaytracingPathNode *node1,
        const SimpleRaytracingPathNode *node2);

    static bool
    eyeNodeVisible(
        const Camera *camera,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        const SimpleRaytracingPathNode *eyeNode,
        const SimpleRaytracingPathNode *node,
        float *pixX,
        float *pixY);

  private:
    static int pathFrontHitFlags();

    static RayHit *
    traceWorld(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        RpkRay *ray,
        Patch *patch,
        unsigned int flags,
        Patch *extraPatch,
        RayHit *hitStore);
};

#endif
