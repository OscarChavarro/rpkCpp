/**
Generate and trace a local line
*/

#include "scene/scene.h"
#include "scene/spherical.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/localline.h"

/**
Constructs a ray with uniformly chosen origin on patch and cosine distributed
direction w.r.t. patch normal. Origin and direction are uniquely determined by
the 4-dimensional sample vector xi
*/
Ray
mcrGenerateLocalLine(Patch *patch, double *xi) {
    static Patch *previousPatch = nullptr;
    static COORDSYS coordSys;
    Ray ray;
    double pdf;

    if ( patch != previousPatch ) {
        // Some work that does not need to be done over if the current patch is the
        // same as the previous one, which is often the case
        patchCoordSys(patch, &coordSys);
        previousPatch = patch;
    }

    patch->uniformPoint(xi[0], xi[1], &ray.pos);
    ray.dir = sampleHemisphereCosTheta(&coordSys, xi[2], xi[3], &pdf);

    return ray;
}

/**
In order to let the user have the impression that the computations are proceeding
*/
static void
someFeedback() {
    if ( (GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays + GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays) % 1000 == 0 ) {
        fputc('.', stderr);
    }
}

/**
Determines nearest intersection point and patch
*/
RayHit *
mcrShootRay(Patch *P, Ray *ray, RayHit *hitStore) {
    float dist = HUGE;
    RayHit *hit;

    // Reject self-intersections
    patchDontIntersect(2, P, P->twin);
    hit = GLOBAL_scene_worldVoxelGrid->gridIntersect(ray, EPSILON < P->tolerance ? EPSILON : P->tolerance, &dist, HIT_FRONT | HIT_POINT,
                                                     hitStore);
    patchDontIntersect(0);
    someFeedback();

    return hit;
}
