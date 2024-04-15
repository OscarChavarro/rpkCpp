/**
Generate and trace a local line
*/

#include "scene/scene.h"
#include "common/mymath.h"
#include "material/spherical.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/localline.h"

/**
Creates a coordinate system on the patch P with Z direction along the normal
*/
static void
patchCoordSys(Patch *patch, CoordSys *coord) {
    coord->Z = patch->normal;
    vectorSubtract(*patch->vertex[1]->point, *patch->vertex[0]->point, coord->X);
    vectorNormalize(coord->X);
    vectorCrossProduct(coord->Z, coord->X, coord->Y);
}

/**
Constructs a ray with uniformly chosen origin on patch and cosine distributed
direction with respect to patch normal. Origin and direction are uniquely determined
by the 4-dimensional sample vector xi
*/
Ray
mcrGenerateLocalLine(Patch *patch, double *xi) {
    static Patch *previousPatch = nullptr;
    static CoordSys coordSys;
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
mcrShootRay(VoxelGrid * sceneWorldVoxelGrid, Patch *P, Ray *ray, RayHit *hitStore) {
    float dist = HUGE;
    RayHit *hit;

    // Reject self-intersections
    Patch::dontIntersect(2, P, P->twin);
    hit = sceneWorldVoxelGrid->gridIntersect(
        ray,
        EPSILON < P->tolerance ? EPSILON : P->tolerance,
        &dist,
        HIT_FRONT | HIT_POINT,
        hitStore);
    Patch::dontIntersect(0);
    someFeedback();

    return hit;
}
