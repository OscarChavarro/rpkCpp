#include <cmath>

#include "scene/scene.h"
#include "scene/VoxelGrid.h"
#include "shared/camera.h"
#include "skin/Patch.h"
#include "raycasting/common/Raytracer.h"
#include "raycasting/common/raytools.h"

bool interrupt_raytracing;

#define PATH_FRONT_HITFLAGS (HIT_FRONT|HIT_POINT|HIT_MATERIAL)
#define PATH_FRONT_BACK_HITFLAGS (PATH_FRONT_HITFLAGS|HIT_BACK)

static HITREC *
TraceWorld(
    Ray *ray,
    PATCH *patch,
    unsigned int flags = PATH_FRONT_HITFLAGS,
    PATCH *extraPatch = nullptr,
    HITREC *hitstore = nullptr)
{
    static HITREC myhitstore;
    float dist;
    HITREC *result;
    if ( !hitstore ) {
        hitstore = &myhitstore;
    }

    dist = HUGE;
    patchDontIntersect(3, patch, patch ? patch->twin : nullptr,
                       extraPatch);
    result = GLOBAL_scene_worldVoxelGrid->gridIntersect(ray, 0., &dist, flags, hitstore);

    if ( result ) {
        /* compute shading frame (Z-axis = shading normal) at intersection point */
        HitShadingFrame(result, &result->X, &result->Y, &result->Z);
    }

    // Disable phong interpolation of normals
    /*
    if(result)
    {
      result->normal = result->patch->normal;
    }
    */

    patchDontIntersect(0);

    return result;
}

HITREC *
FindRayIntersection(
    Ray *ray,
    PATCH *patch,
    BSDF *currentBsdf,
    HITREC *hitstore)
{
    int hitFlags;
    HITREC *newHit;

    if ( currentBsdf == nullptr ) {
        /* outside everything in vacuum */
        hitFlags = PATH_FRONT_HITFLAGS;
    } else {
        hitFlags = PATH_FRONT_BACK_HITFLAGS;
    }

    // Trace the ray
    newHit = TraceWorld(ray, patch, hitFlags, nullptr, hitstore);
    Global_Raytracer_rayCount++; // statistics

    // Robustness test : If a back is hit, check the current
    // bsdf and the bsdf of the material hit. If they
    // don't match, exclude this patch and trace again :-(

    if ( newHit != nullptr && (newHit->flags & HIT_BACK)) {
        if ( newHit->patch->surface->material->bsdf != currentBsdf ) {
            // Whoops, intersected with wrong patch (accuracy problem)

            // Warning("Findrayintersection", "Wrong patch : accuracy");

            newHit = TraceWorld(ray, patch,
                                hitFlags, newHit->patch, hitstore);
            Global_Raytracer_rayCount++; // statistics
        }
    }

    return newHit;
}


/**
PathNodesVisible : send a shadowray
*/
bool
PathNodesVisible(CPathNode *node1, CPathNode *node2) {
    Vector3D dir;
    Ray ray;
    HITREC *hit, hitstore;
    double cosRay1, cosRay2, dist, dist2;
    float fdist;
    bool visible = false, doTest;

    // Determines visibility between two nodes,
    // Returns visibility and direction from eye to light node (newDir_e)

    if ( node1->m_hit.patch == node2->m_hit.patch ) {
        // Same patch cannot see itself. Wrong for concave primitives !!

        //printf("H");
        return false;
    }

    VECTORSUBTRACT(node2->m_hit.point, node1->m_hit.point,
                   dir);

    dist2 = VECTORNORM2(dir);
    dist = sqrt(dist2);

    VECTORSCALEINVERSE(dist, dir, dir);

    // *newDir_e = dir;

    dist = dist * (1 - EPSILON);

    ray.pos = node1->m_hit.point;
    VECTORCOPY(dir, ray.dir);

    cosRay1 = VECTORDOTPRODUCT(dir, node1->m_normal);
    cosRay2 = -VECTORDOTPRODUCT(dir, node2->m_normal);

    doTest = false;

    if ( cosRay1 > 0 ) {
        if ( cosRay2 > 0 ) {
            // Normal case : reflected rays.
            doTest = true;
        } else {
            // node1 reflects, node2 transmits

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
        if ( node2->m_hit.patch->isVirtual()) {
            fdist = HUGE;
        } else {
            fdist = (float) dist;
        }

        patchDontIntersect(3, node2->m_hit.patch, node1->m_hit.patch,
                           node1->m_hit.patch ? node1->m_hit.patch->twin : nullptr);
        hit = GLOBAL_scene_worldVoxelGrid->gridIntersect(&ray,
                            0., &fdist,
                            HIT_FRONT | HIT_BACK | HIT_ANY, &hitstore);
        patchDontIntersect(0);
        visible = (hit == nullptr);

        Global_Raytracer_rayCount++; // statistics

        // geomFactor

        // *geomFactor = cosRayEye * cosRayLight / dist2;
    } else {
        visible = false;
    }

    return (visible);
}

/**
Can the eye see the node ?  If so, pix_x and pix_y are filled in
*/
bool
EyeNodeVisible(
    CPathNode *eyeNode,
    CPathNode *node,
    float *pix_x,
    float *pix_y)
{
    Vector3Dd dir;
    Ray ray;
    HITREC *hit, hitstore;
    double cosRayLight, cosRayEye, dist, dist2;
    float fdist;
    bool visible = false;
    double x, y, z, xz, yz;
    // double xn, yn, zn;

    // Determines visibility between two nodes,
    // Returns visibility and direction from eye to light node (newDir_e)

    VECTORSUBTRACT(node->m_hit.point, eyeNode->m_hit.point,
                   dir);

    dist2 = VECTORNORM2(dir);
    dist = sqrt(dist2);

    VECTORSCALEINVERSE(dist, dir, dir);

    //  *newDir_e = dir;

    // Determine which pixel is visible

    z = VECTORDOTPRODUCT(dir, GLOBAL_camera_mainCamera.Z);
    // zn = VECTORDOTPRODUCT(node->m_normal,GLOBAL_camera_mainCamera.Z);
    // xn = VECTORDOTPRODUCT(node->m_normal,GLOBAL_camera_mainCamera.X);
    // yn = VECTORDOTPRODUCT(node->m_normal,GLOBAL_camera_mainCamera.Y);

    visible = false;

    // || (fabs(xn/zn) > tanhfov) || (fabs(yn/zn) > tanvfov) ) )
    if ( z > 0.0 ) {
        x = VECTORDOTPRODUCT(dir, GLOBAL_camera_mainCamera.X);
        xz = x / z;

        if ( fabs(xz) < GLOBAL_camera_mainCamera.tanhfov ) {
            y = VECTORDOTPRODUCT(dir, GLOBAL_camera_mainCamera.Y);
            yz = y / z;

            if ( fabs(yz) < GLOBAL_camera_mainCamera.tanvfov ) {
                /* point is within view piramid */

                /* check normal directions */

                dist = dist * (1 - EPSILON);

                ray.pos = eyeNode->m_hit.point;
                VECTORCOPY(dir, ray.dir);

                cosRayEye = VECTORDOTPRODUCT(dir, eyeNode->m_normal);
                cosRayLight = -VECTORDOTPRODUCT(dir, node->m_normal);

                if ((cosRayLight > 0) && (cosRayEye > 0)) {
                    fdist = (float) dist;
                    patchDontIntersect(3, node->m_hit.patch, eyeNode->m_hit.patch,
                                       eyeNode->m_hit.patch ? eyeNode->m_hit.patch->twin : nullptr);
                    hit = GLOBAL_scene_worldVoxelGrid->gridIntersect(&ray,
                                        0., &fdist,
                                        HIT_FRONT | HIT_ANY, &hitstore);
                    patchDontIntersect(0);
                    // HIT_BACK removed ! So you can see through backwalls with N.E.E
                    visible = (hit == nullptr);

                    // geomFactor

                    if ( visible ) {
                        // *geomFactor = cosRayEye * cosRayLight / dist2;

                        *pix_x = xz;
                        *pix_y = yz;

                        //*nx = (int)floor((1.0 + xz/GLOBAL_camera_mainCamera.tanhfov)/2.0 * GLOBAL_camera_mainCamera.hres);
                        //*ny = (int)floor((1.0 + yz/GLOBAL_camera_mainCamera.tanvfov)/2.0 * GLOBAL_camera_mainCamera.vres);


                    }
                }
            }
        }
    }

    return (visible);
}
