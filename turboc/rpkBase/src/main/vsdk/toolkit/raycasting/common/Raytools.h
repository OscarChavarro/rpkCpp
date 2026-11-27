/**
Some utility routines for ray intersections and for statistics
*/

#ifndef __RAYTOOLS__
#define __RAYTOOLS__

#include "vsdk/toolkit/environment/geometry/element/Ray.h"
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
        Ray *ray,
        Patch *patch,
        const PhongBidirScattDistFunc *currentBsdf,
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
        double *pixX,
        double *pixY);

  private:
    static int pathFrontHitFlags();

    static RpkRayHit *
    traceWorld(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        Ray *ray,
        Patch *patch,
        unsigned int flags,
        Patch *extraPatch,
        RpkRayHit *hitStore);
};

#endif
