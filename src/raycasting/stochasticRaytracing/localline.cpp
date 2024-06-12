/**
Generate and trace a local line
*/

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "common/linealAlgebra/CoordinateSystem.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/localline.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

/**
Creates a coordinate system on the patch P with Z direction along the normal
*/
static void
patchCoordSys(const Patch *patch, CoordinateSystem *coord) {
    coord->Z = patch->normal;
    coord->X.subtraction(*patch->vertex[1]->point, *patch->vertex[0]->point);
    coord->X.normalize(Numeric::EPSILON_FLOAT);
    coord->Y.crossProduct(coord->Z, coord->X);
}

/**
Constructs a ray with uniformly chosen origin on patch and cosine distributed
direction with respect to patch normal. Origin and direction are uniquely determined
by the 4-dimensional sample vector xi
*/
Ray
mcrGenerateLocalLine(const Patch *patch, const double *xi) {
    const static Patch *previousPatch = nullptr;
    static CoordinateSystem coordSys;
    Ray ray;
    double pdf;

    if ( patch != previousPatch ) {
        // Some work that does not need to be done over if the current patch is the
        // same as the previous one, which is often the case
        patchCoordSys(patch, &coordSys);
        previousPatch = patch;
    }

    patch->uniformPoint(xi[0], xi[1], &ray.pos);
    ray.dir = coordSys.sampleHemisphereCosTheta(xi[2], xi[3], &pdf);

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
mcrShootRay(const VoxelGrid * sceneWorldVoxelGrid, Patch *P, Ray *ray, RayHit *hitStore) {
    float distance = Numeric::HUGE_FLOAT_VALUE;
    RayHit *hit;

    // Reject self-intersections
    Patch::dontIntersect(2, P, P->twin);
    hit = sceneWorldVoxelGrid->gridIntersect(
        ray,
        Numeric::EPSILON_FLOAT < P->tolerance ? Numeric::EPSILON_FLOAT : P->tolerance,
        &distance,
        RayHitFlag::FRONT | RayHitFlag::POINT,
        hitStore);
    Patch::dontIntersect(0);
    someFeedback();

    return hit;
}

#endif
