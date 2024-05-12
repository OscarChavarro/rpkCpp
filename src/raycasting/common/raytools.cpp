#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "common/mymath.h"
#include "scene/Camera.h"
#include "raycasting/common/Raytracer.h"
#include "raycasting/common/raytools.h"

#define PATH_FRONT_HIT_FLAGS (HIT_FRONT|HIT_POINT|HIT_MATERIAL)
#define PATH_FRONT_BACK_HIT_FLAGS (PATH_FRONT_HIT_FLAGS|HIT_BACK)

static RayHit *
traceWorld(
    const VoxelGrid *sceneWorldVoxelGrid,
    Ray *ray,
    Patch *patch,
    unsigned int flags = PATH_FRONT_HIT_FLAGS,
    Patch *extraPatch = nullptr,
    RayHit *hitStore = nullptr)
{
    static RayHit myHitStore;
    float dist;
    RayHit *result;
    if ( !hitStore ) {
        hitStore = &myHitStore;
    }

    dist = HUGE_FLOAT;
    Patch::dontIntersect(3, patch, patch ? patch->twin : nullptr, extraPatch);
    result = sceneWorldVoxelGrid->gridIntersect(ray, 0.0, &dist, (int)flags, hitStore);

    if ( result ) {
        // Compute shading frame (Z-axis = shading normal) at intersection point
        CoordinateSystem frame = result->getShadingFrame();
        result->setShadingFrame(&frame);
    }
    Patch::dontIntersect(0);

    return result;
}

RayHit *
findRayIntersection(
    const VoxelGrid *sceneWorldVoxelGrid,
    Ray *ray,
    Patch *patch,
    const PhongBidirectionalScatteringDistributionFunction *currentBsdf,
    RayHit *hitStore)
{
    int hitFlags;
    RayHit *newHit;

    if ( currentBsdf == nullptr ) {
        // Outside everything in vacuum
        hitFlags = PATH_FRONT_HIT_FLAGS;
    } else {
        hitFlags = PATH_FRONT_BACK_HIT_FLAGS;
    }

    // Trace the ray
    newHit = traceWorld(sceneWorldVoxelGrid, ray, patch, hitFlags, nullptr, hitStore);
    GLOBAL_raytracer_rayCount++; // statistics

    // Robustness test : If a back is hit, check the current
    // bsdf and the bsdf of the material hit. If they
    // don't match, exclude this patch and trace again :-(
    if ( newHit != nullptr && (newHit->getFlags() & HIT_BACK) &&
         newHit->getPatch()->material->getBsdf() != currentBsdf ) {
        // Whoops, intersected with wrong patch (accuracy problem)
        newHit = traceWorld(sceneWorldVoxelGrid, ray, patch, hitFlags, newHit->getPatch(), hitStore);
        GLOBAL_raytracer_rayCount++; // Statistics
    }

    return newHit;
}

/**
pathNodesVisible : send a shadow ray
*/
bool
pathNodesVisible(
    const VoxelGrid *sceneWorldVoxelGrid,
    const SimpleRaytracingPathNode *node1,
    const SimpleRaytracingPathNode *node2)
{
    Vector3D dir;
    Ray ray;
    const RayHit *hit;
    RayHit hitStore;
    double cosRay1;
    double cosRay2;
    double dist;
    double dist2;
    float fDistance;
    bool visible;
    bool doTest;

    // Determines visibility between two nodes,
    // Returns visibility and direction from eye to light node (newDir_e)
    if ( node1->m_hit.getPatch() == node2->m_hit.getPatch() ) {
        // Same patch cannot see itself. Wrong for concave primitives!
        return false;
    }

    dir.subtraction(node2->m_hit.getPoint(), node1->m_hit.getPoint());

    dist2 = dir.norm2();
    dist = std::sqrt(dist2);

    dir.inverseScaledCopy((float)dist, dir, EPSILON_FLOAT);

    dist = dist * (1 - EPSILON);

    ray.pos = node1->m_hit.getPoint();
    ray.dir.copy(dir);

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
            fDistance = HUGE_FLOAT;
        } else {
            fDistance = (float) dist;
        }

        Patch::dontIntersect(
            3,
            node2->m_hit.getPatch(),
            node1->m_hit.getPatch(),
            node1->m_hit.getPatch() != nullptr ? node1->m_hit.getPatch()->twin : nullptr);
        hit = sceneWorldVoxelGrid->gridIntersect(
            &ray,
            0.0,
            &fDistance,
            HIT_FRONT | HIT_BACK | HIT_ANY,
            &hitStore);
        Patch::dontIntersect(0);
        visible = (hit == nullptr);

        GLOBAL_raytracer_rayCount++; // Statistics
    } else {
        visible = false;
    }

    return visible;
}

/**
Can the eye see the node ?  If so, pix_x and pix_y are filled in
*/
bool
eyeNodeVisible(
    const Camera *camera,
    const VoxelGrid *sceneWorldVoxelGrid,
    const SimpleRaytracingPathNode *eyeNode,
    const SimpleRaytracingPathNode *node,
    float *pixX,
    float *pixY)
{
    Vector3D dir;
    Ray ray;
    const RayHit *hit;
    RayHit hitStore;
    double cosRayLight;
    double cosRayEye;
    double dist;
    double dist2;
    float fDistance;
    double x;
    double y;
    double z;
    double xz;
    double yz;

    // Determines visibility between two nodes,
    // Returns visibility and direction from eye to light node (newDir_e)
    dir.subtraction(node->m_hit.getPoint(), eyeNode->m_hit.getPoint());

    dist2 = dir.norm2();
    dist = std::sqrt(dist2);

    dir.inverseScaledCopy((float)dist, dir, EPSILON_FLOAT);

    // Determine which pixel is visible
    z = dir.dotProduct(camera->Z);

    bool visible = false;
    if ( z > 0.0 ) {
        x = dir.dotProduct(camera->X);
        xz = x / z;

        if ( std::fabs(xz) < camera->pixelWidthTangent ) {
            y = dir.dotProduct(camera->Y);
            yz = y / z;

            if ( std::fabs(yz) < camera->pixelHeightTangent ) {
                // Point is within view pyramid

                // Check normal directions
                dist = dist * (1 - EPSILON);

                ray.pos = eyeNode->m_hit.getPoint();
                ray.dir.copy(dir);

                cosRayEye = dir.dotProduct(eyeNode->m_normal);
                cosRayLight = -dir.dotProduct(node->m_normal);

                if ( (cosRayLight > 0) && (cosRayEye > 0) ) {
                    fDistance = (float) dist;
                    Patch::dontIntersect(
                        3, node->m_hit.getPatch(), eyeNode->m_hit.getPatch(),
                         eyeNode->m_hit.getPatch() ? eyeNode->m_hit.getPatch()->twin : nullptr);
                    hit = sceneWorldVoxelGrid->gridIntersect(&ray,
                        0.0, &fDistance,
                        HIT_FRONT | HIT_ANY, &hitStore);
                    Patch::dontIntersect(0);
                    // HIT_BACK removed ! So you can see through back walls with N.E.E
                    visible = (hit == nullptr);

                    // Geometry factor
                    if ( visible ) {
                        *pixX = (float)xz;
                        *pixY = (float)yz;
                    }
                }
            }
        }
    }

    return visible;
}

#endif
