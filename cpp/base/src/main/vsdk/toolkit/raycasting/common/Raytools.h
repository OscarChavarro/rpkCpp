/**
Some utility routines for ray intersections and for statistics
*/

#ifndef RAY_TOOLS__
#define RAY_TOOLS__

#include "vsdk/toolkit/common/linealAlgebra/RpkRay.h"
#include "vsdk/toolkit/material/PhongBidirectionalScatteringDistributionFunction.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/raycasting/common/SimpleRaytracingPathNode.h"

class RayTools {
  public:
    static RpkRayHit *
    findRayIntersection(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        RpkRay *ray,
        const Patch *patch,
        const PhongBidirectionalScatteringDistributionFunction *currentBsdf,
        RpkRayHit *hitStore);

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

    static RpkRayHit *
    traceWorld(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        RpkRay *ray,
        const Patch *patch,
        unsigned int flags,
        const Patch *extraPatch,
        RpkRayHit *hitStore);
};

#endif
