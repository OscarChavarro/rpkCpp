/*
generate and trace a local line
*/

#include "scene/scene.h"
#include "scene/spherical.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/localline.h"

/* constructs a ray with uniformly chosen origin on patch and cosine distributed 
 * direction w.r.t. patch normal. Origin and direction are uniquely determined by
 * the 4-dimensional sample vector xi. */
Ray mcrGenerateLocalLine(Patch *patch, double *xi) {
    static Patch *prevpatch = (Patch *) nullptr;
    static COORDSYS coordsys;
    Ray ray;
    double pdf;

    if ( patch != prevpatch ) {
        /* some work that does not need to be done over if the current patch is the
         * same as the previous one, which is often the case. */
        patchCoordSys(patch, &coordsys);
        prevpatch = patch;
    }

    patchUniformPoint(patch, xi[0], xi[1], &ray.pos);
    ray.dir = sampleHemisphereCosTheta(&coordsys, xi[2], xi[3], &pdf);

    return ray;
}

/* in order to let the user have the impression that the computations are proceeding. */
static void SomeFeedback() {
    if ((GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays + GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays) % 1000 == 0 ) {
        fputc('.', stderr);
    }
}

/**
Determines nearest intersection point and patch
*/
RayHit *
mcrShootRay(Patch *P, Ray *ray, RayHit *hitstore) {
    float dist = HUGE;
    RayHit *hit;

    /* reject selfintersections */
    patchDontIntersect(2, P, P->twin);
    hit = GLOBAL_scene_worldVoxelGrid->gridIntersect(ray, EPSILON < P->tolerance ? EPSILON : P->tolerance, &dist, HIT_FRONT | HIT_POINT,
                        hitstore);
    patchDontIntersect(0);
    SomeFeedback();

    return hit;
}
