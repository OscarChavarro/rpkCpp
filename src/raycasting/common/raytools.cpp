#include "scene/scene.h"
#include "scene/Camera.h"
#include "raycasting/common/Raytracer.h"
#include "raycasting/common/raytools.h"

bool GLOBAL_rayCasting_interruptRaytracing;

#define PATH_FRONT_HIT_FLAGS (HIT_FRONT|HIT_POINT|HIT_MATERIAL)
#define PATH_FRONT_BACK_HIT_FLAGS (PATH_FRONT_HIT_FLAGS|HIT_BACK)

static RayHit *
traceWorld(
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

    dist = HUGE;
    Patch::dontIntersect(3, patch, patch ? patch->twin : nullptr, extraPatch);
    result = GLOBAL_scene_worldVoxelGrid->gridIntersect(ray, 0.0, &dist, (int)flags, hitStore);

    if ( result ) {
        // Compute shading frame (Z-axis = shading normal) at intersection point
        hitShadingFrame(result, &result->X, &result->Y, &result->Z);
    }

    Patch::dontIntersect(0);

    return result;
}

RayHit *
findRayIntersection(
    Ray *ray,
    Patch *patch,
    BSDF *currentBsdf,
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
    newHit = traceWorld(ray, patch, hitFlags, nullptr, hitStore);
    GLOBAL_raytracer_rayCount++; // statistics

    // Robustness test : If a back is hit, check the current
    // bsdf and the bsdf of the material hit. If they
    // don't match, exclude this patch and trace again :-(
    if ( newHit != nullptr && (newHit->flags & HIT_BACK)) {
        if ( newHit->patch->surface->material->bsdf != currentBsdf ) {
            // Whoops, intersected with wrong patch (accuracy problem)
            newHit = traceWorld(ray, patch, hitFlags, newHit->patch, hitStore);
            GLOBAL_raytracer_rayCount++; // Statistics
        }
    }

    return newHit;
}

/**
pathNodesVisible : send a shadow ray
*/
bool
pathNodesVisible(CPathNode *node1, CPathNode *node2) {
    Vector3D dir;
    Ray ray;
    RayHit *hit;
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
    if ( node1->m_hit.patch == node2->m_hit.patch ) {
        // Same patch cannot see itself. Wrong for concave primitives!
        return false;
    }

    vectorSubtract(node2->m_hit.point, node1->m_hit.point,
                   dir);

    dist2 = vectorNorm2(dir);
    dist = sqrt(dist2);

    vectorScaleInverse((float) dist, dir, dir);

    dist = dist * (1 - EPSILON);

    ray.pos = node1->m_hit.point;
    vectorCopy(dir, ray.dir);

    cosRay1 = vectorDotProduct(dir, node1->m_normal);
    cosRay2 = -vectorDotProduct(dir, node2->m_normal);

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
        if ( node2->m_hit.patch->hasZeroVertices()) {
            fDistance = HUGE;
        } else {
            fDistance = (float) dist;
        }

        Patch::dontIntersect(3, node2->m_hit.patch, node1->m_hit.patch,
                             node1->m_hit.patch ? node1->m_hit.patch->twin : nullptr);
        hit = GLOBAL_scene_worldVoxelGrid->gridIntersect(&ray,
                            0., &fDistance,
                            HIT_FRONT | HIT_BACK | HIT_ANY, &hitStore);
        Patch::dontIntersect(0);
        visible = (hit == nullptr);

        GLOBAL_raytracer_rayCount++; // Statistics
    } else {
        visible = false;
    }

    return (visible);
}

/**
Can the eye see the node ?  If so, pix_x and pix_y are filled in
*/
bool
eyeNodeVisible(
    CPathNode *eyeNode,
    CPathNode *node,
    float *pix_x,
    float *pix_y)
{
    Vector3D dir;
    Ray ray;
    RayHit *hit;
    RayHit hitStore;
    double cosRayLight;
    double cosRayEye;
    double dist;
    double dist2;
    float fDistance;
    bool visible;
    double x;
    double y;
    double z;
    double xz;
    double yz;

    // Determines visibility between two nodes,
    // Returns visibility and direction from eye to light node (newDir_e)
    vectorSubtract(node->m_hit.point, eyeNode->m_hit.point, dir);

    dist2 = vectorNorm2(dir);
    dist = std::sqrt(dist2);

    vectorScaleInverse(dist, dir, dir);

    // Determine which pixel is visible

    z = vectorDotProduct(dir, GLOBAL_camera_mainCamera.Z);

    visible = false;

    if ( z > 0.0 ) {
        x = vectorDotProduct(dir, GLOBAL_camera_mainCamera.X);
        xz = x / z;

        if ( std::fabs(xz) < GLOBAL_camera_mainCamera.pixelWidthTangent ) {
            y = vectorDotProduct(dir, GLOBAL_camera_mainCamera.Y);
            yz = y / z;

            if ( std::fabs(yz) < GLOBAL_camera_mainCamera.pixelHeightTangent ) {
                // Point is within view pyramid

                // Check normal directions
                dist = dist * (1 - EPSILON);

                ray.pos = eyeNode->m_hit.point;
                vectorCopy(dir, ray.dir);

                cosRayEye = vectorDotProduct(dir, eyeNode->m_normal);
                cosRayLight = -vectorDotProduct(dir, node->m_normal);

                if ( (cosRayLight > 0) && (cosRayEye > 0) ) {
                    fDistance = (float) dist;
                    Patch::dontIntersect(3, node->m_hit.patch, eyeNode->m_hit.patch,
                                         eyeNode->m_hit.patch ? eyeNode->m_hit.patch->twin : nullptr);
                    hit = GLOBAL_scene_worldVoxelGrid->gridIntersect(&ray,
                                        0., &fDistance,
                                        HIT_FRONT | HIT_ANY, &hitStore);
                    Patch::dontIntersect(0);
                    // HIT_BACK removed ! So you can see through backwalls with N.E.E
                    visible = (hit == nullptr);

                    // geomFactor

                    if ( visible ) {
                        // *geomFactor = cosRayEye * cosRayLight / dist2;
                        *pix_x = (float)xz;
                        *pix_y = (float)yz;
                    }
                }
            }
        }
    }

    return visible;
}
