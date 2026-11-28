#include "java/lang/Double.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/raycasting/common/Raytools.h"


void
RayTools::countRay(SceneQueryWorkspace *workspace) {
    long *count = workspace->getRayCount();
    if ( count != nullptr ) {
        ++*count;
    } else {
        Statistics::instance().rayTracer.rayCount++;
    }
}

int
RayTools::pathFrontHitFlags() {
    return RayHitFlag::FRONT | RayHitFlag::POINT | RayHitFlag::MATERIAL;
}

RpkRayHit *
RayTools::traceWorld(
    const SceneQuery *sceneQuery,
    SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
    Ray *ray,
    const Patch *patch,
    unsigned int flags,
    const Patch *extraPatch,
    RpkRayHit *hitStore)
{
    static thread_local RpkRayHit myHitStore;
    double dist;
    RpkRayHit *result;
    if ( !hitStore ) {
        hitStore = &myHitStore;
    }

    dist = java::Double::MAX_VALUE;
    const RayExclusionSet exclusions(patch, patch ? patch->getTwin() : nullptr, extraPatch);
    result = sceneQuery->intersect(ray, 0.0, &dist, static_cast<int>(flags), hitStore, exclusions, sceneWorldSceneQueryWorkspace);

    if ( result ) {
        // Compute shading frame (Z-axis = shading normal) at intersection point
        CoordinateSystem frame = result->getShadingFrame();
        result->setShadingFrame(&frame);
    }

    return result;
}

RpkRayHit *
RayTools::findRayIntersection(
    const SceneQuery *sceneQuery,
    SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
    Ray *ray,
    const Patch *patch,
    const PhongBidirectionalScatteringDistributionFunction *currentBsdf,
    RpkRayHit *hitStore)
{
    int hitFlags;
    RpkRayHit *newHit;

    if ( currentBsdf == nullptr ) {
        // Outside everything in vacuum
        hitFlags = pathFrontHitFlags();
    } else {
        hitFlags = pathFrontHitFlags() | RayHitFlag::BACK;
    }

    // Trace the ray
    newHit = traceWorld(sceneQuery, sceneWorldSceneQueryWorkspace, ray, patch, hitFlags, nullptr, hitStore);
    countRay(sceneWorldSceneQueryWorkspace);

    // Robustness test : If a back is hit, check the current
    // bsdf and the bsdf of the material hit. If they
    // don't match, exclude this patch and trace again :-(
    if ( newHit != nullptr && (newHit->getFlags() & RayHitFlag::BACK) &&
         newHit->getPatch()->getMaterial()->getBsdf() != currentBsdf ) {
        // Whoops, intersected with wrong patch (accuracy problem)
        newHit = traceWorld(sceneQuery, sceneWorldSceneQueryWorkspace, ray, patch, hitFlags, newHit->getPatch(), hitStore);
        countRay(sceneWorldSceneQueryWorkspace);
    }

    return newHit;
}

/**
pathNodesVisible : send a shadow ray
*/
bool
RayTools::pathNodesVisible(
    const SceneQuery *sceneQuery,
    SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
    const SimpleRaytracingPathNode *node1,
    const SimpleRaytracingPathNode *node2)
{
    Vector3Dd dir;
    Ray ray;
    const RpkRayHit *hit;
    RpkRayHit hitStore;
    double cosRay1;
    double cosRay2;
    double dist;
    double dist2;
    double fDistance;
    bool visible;
    bool doTest;

    // Determines visibility between two nodes,
    // Returns visibility and direction from eye to light node (newDir_e)
    if ( node1->m_hit.getPatch() == node2->m_hit.getPatch() ) {
        // Same patch cannot see itself. Wrong for concave primitives!
        return false;
    }

    dir = (node2->m_hit.getPoint()).subtract(node1->m_hit.getPoint());

    dist2 = dir.lengthSquared();
    dist = java::Math::sqrt(dist2);

    dir = dir.multiplyByInverse(static_cast<double>(dist), Numeric::EPSILON);

    dist = dist * (1 - VSDK::EPSILON);

    ray.setOrigin(node1->m_hit.getPoint());
    ray.setDirection(dir);

    cosRay1 = dir.dotProduct(node1->m_normal);
    cosRay2 = -dir.dotProduct(node2->m_normal);

    doTest = false;

    if ( cosRay1 > 0 ) {
        if ( cosRay2 > 0 ) {
            // Normal case : reflected rays.
            doTest = true;
        } else {
            // Node1 reflects, node2 transmits
            if ( node1->m_inBsdf == node2->m_outBsdf ) {
                doTest = true;
            }
        }
    } else {
        if ( cosRay2 > 0 ) {
            if ( node1->m_outBsdf == node2->m_inBsdf ) {
                doTest = true;
            }
        } else {
            if ( node1->m_outBsdf == node2->m_outBsdf ) {
                doTest = true;
            }
        }
    }

    if ( doTest ) {
        if ( node2->m_hit.getPatch()->hasZeroVertices() ) {
            fDistance = java::Double::MAX_VALUE;
        } else {
            fDistance = static_cast<double>(dist);
        }

        const RayExclusionSet exclusions(
            node2->m_hit.getPatch(),
            node1->m_hit.getPatch(),
            node1->m_hit.getPatch() != nullptr ? node1->m_hit.getPatch()->getTwin() : nullptr);
        hit = sceneQuery->intersect(
            &ray,
            0.0,
            &fDistance,
            RayHitFlag::FRONT | RayHitFlag::BACK | RayHitFlag::ANY,
            &hitStore,
            exclusions,
            sceneWorldSceneQueryWorkspace);
        visible = (hit == nullptr);

        countRay(sceneWorldSceneQueryWorkspace);
    } else {
        visible = false;
    }

    return visible;
}

/**
Can the eye see the node ?  If so, pix_x and pix_y are filled in
*/
bool
RayTools::eyeNodeVisible(
    const Camera *camera,
    const SceneQuery *sceneQuery,
    SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
    const SimpleRaytracingPathNode *eyeNode,
    const SimpleRaytracingPathNode *node,
    double *pixX,
    double *pixY)
{
    Vector3Dd dir;
    Ray ray;
    const RpkRayHit *hit;
    RpkRayHit hitStore;
    double cosRayLight;
    double cosRayEye;
    double dist;
    double dist2;
    double fDistance;
    double x;
    double y;
    double z;
    double xz;
    double yz;

    // Determines visibility between two nodes,
    // Returns visibility and direction from eye to light node (newDir_e)
    dir = (node->m_hit.getPoint()).subtract(eyeNode->m_hit.getPoint());

    dist2 = dir.lengthSquared();
    dist = java::Math::sqrt(dist2);

    dir = dir.multiplyByInverse(static_cast<double>(dist), Numeric::EPSILON);

    // Determine which pixel is visible
    z = dir.dotProduct(camera->Z);

    bool visible = false;
    if ( z > 0.0 ) {
        x = dir.dotProduct(camera->X);
        xz = x / z;

        if ( java::Math::abs(xz) < camera->pixelWidthTangent ) {
            y = dir.dotProduct(camera->Y);
            yz = y / z;

            if ( java::Math::abs(yz) < camera->pixelHeightTangent ) {
                // Point is within view pyramid

                // Check normal directions
                dist = dist * (1 - VSDK::EPSILON);

                ray.setOrigin(eyeNode->m_hit.getPoint());
                ray.setDirection(dir);

                cosRayEye = dir.dotProduct(eyeNode->m_normal);
                cosRayLight = -dir.dotProduct(node->m_normal);

                if ( (cosRayLight > 0) && (cosRayEye > 0) ) {
                    fDistance = static_cast<double>(dist);
                    const RayExclusionSet exclusions(
                        node->m_hit.getPatch(), eyeNode->m_hit.getPatch(),
                        eyeNode->m_hit.getPatch() ? eyeNode->m_hit.getPatch()->getTwin() : nullptr);
                    hit = sceneQuery->intersect(&ray,
                                                             0.0, &fDistance,
                                                             RayHitFlag::FRONT | RayHitFlag::ANY, &hitStore,
                                                             exclusions, sceneWorldSceneQueryWorkspace);
                    // HIT_BACK removed ! So you can see through back walls with N.E.E
                    visible = (hit == nullptr);

                    // Geometry factor
                    if ( visible ) {
                        *pixX = static_cast<double>(xz);
                        *pixY = static_cast<double>(yz);
                    }
                }
            }
        }
    }

    return visible;
}

#endif
