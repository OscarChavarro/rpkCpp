/**
Generate and trace a local line
*/
#include "java/lang/System.h"
#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "common/linealAlgebra/CoordinateSystem.h"
#include "raycasting/stochasticRaytracing/McradP.h"
#include "raycasting/stochasticRaytracing/Localline.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

/**
Creates a coordinate system on the patch P with Z direction along the normal
*/
void
Localline::patchCoordSys(const Patch *patch, CoordinateSystem *coord) {
    Vector3D z = patch->normal;
    Vector3D x;
    x.subtraction(*patch->vertex[1]->point, *patch->vertex[0]->point);
    x.normalize(Numeric::EPSILON_FLOAT);
    Vector3D y;
    y.crossProduct(z, x);

    coord->setX(x);
    coord->setY(y);
    coord->setZ(z);
}

/**
Constructs a ray with uniformly chosen origin on patch and cosine distributed
direction with respect to patch normal. Origin and direction are uniquely determined
by the 4-dimensional sample vector xi
*/
Ray
Localline::mcrGenerateLocalLine(const Patch *patch, const double *xi) {
    const static Patch *previousPatch = NULL;
    static CoordinateSystem coordSys;
    Ray ray;
    double pdf;

    if ( patch != previousPatch ) {
        // Some work that does not need to be done over if the current patch is the
        // same as the previous one, which is often the case
        patchCoordSys(patch, &coordSys);
        previousPatch = patch;
    }

    patch->uniformPoint(xi[0], xi[1], &ray.position);
    // Section [ARVO1995b].2: use two uniform samples from [0,1]^2 to
    // generate a local hemisphere direction in the patch frame.
    ray.direction = coordSys.sampleHemisphereCosTheta(xi[2], xi[3], &pdf);

    return ray;
}

/**
In order to let the user have the impression that the computations are proceeding
*/
void
Localline::someFeedback() {
    if ( (StochasticRelaxation::activeState().tracedRays + StochasticRelaxation::activeState().importanceTracedRays) % 1000 == 0 ) {
        System::err.print(".");
    }
}

/**
Determines nearest intersection point and patch
*/
RayHit *
Localline::mcrShootRay(const VoxelGrid *sceneWorldVoxelGrid, Patch *P, Ray *ray, RayHit *hitStore) {
    float distance = Numeric::HUGE_FLOAT_VALUE;
    RayHit *hit;

    // Reject self-intersections
    Patch::dontIntersect2(P, P->twin);
    hit = sceneWorldVoxelGrid->gridIntersect(
        ray,
        Numeric::EPSILON_FLOAT < P->tolerance ? Numeric::EPSILON_FLOAT : P->tolerance,
        &distance,
        FRONT | POINT,
        hitStore);
    Patch::dontIntersect0();
    someFeedback();

    return hit;
}

#endif
