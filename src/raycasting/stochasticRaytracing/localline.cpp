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
Ray McrGenerateLocalLine(PATCH *patch, double *xi) {
    static PATCH *prevpatch = (PATCH *) nullptr;
    static COORDSYS coordsys;
    Ray ray;
    double pdf;

    if ( patch != prevpatch ) {
        /* some work that does not need to be done over if the current patch is the
         * same as the previous one, which is often the case. */
        PatchCoordSys(patch, &coordsys);
        prevpatch = patch;
    }

    PatchUniformPoint(patch, xi[0], xi[1], &ray.pos);
    ray.dir = SampleHemisphereCosTheta(&coordsys, xi[2], xi[3], &pdf);

    return ray;
}

/* in order to let the user have the impression that the computations are proceeding. */
static void SomeFeedback() {
    if ((GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays + GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays) % 1000 == 0 ) {
        fputc('.', stderr);
    }
}

/* determines nearest intersection point and patch */
HITREC *McrShootRay(PATCH *P, Ray *ray, HITREC *hitstore) {
    float dist = HUGE;
    HITREC *hit;

    /* reject selfintersections */
    PatchDontIntersect(2, P, P->twin);
    hit = GridIntersect(GLOBAL_scene_worldGrid, ray, EPSILON < P->tolerance ? EPSILON : P->tolerance, &dist, HIT_FRONT | HIT_POINT,
                        hitstore);
    PatchDontIntersect(0);
    SomeFeedback();

    return hit;
}
