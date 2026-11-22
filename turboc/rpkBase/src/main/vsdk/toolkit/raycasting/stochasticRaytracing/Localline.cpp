#include "java/lang/Float.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
/**
Generate and trace a local line
*/
#include "java/lang/System.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "vsdk/toolkit/common/linealAlgebra/CoordinateSystem.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/McradP.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Localline.h"

#ifdef RPK_RAY_CAPTURE
    #include "vsdk/toolkit/scene/VisibilityQueryRecorder.h"
#endif
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation.h"

/**
Creates a coordinate system on the patch P with Z direction along the normal
*/
void
Localline::patchCoordSys(const Patch *patch, CoordinateSystem *coord) {
    Vector3Df z = patch->normal;
    Vector3Df x;
    x = (*patch->vertex[1]->point).subtract(*patch->vertex[0]->point);
    x = x.normalized(Numeric::EPSILON_FLOAT);
    Vector3Df y;
    y = z.crossProduct(x);

    coord->setX(x);
    coord->setY(y);
    coord->setZ(z);
}

/**
Constructs a ray with uniformly chosen origin on patch and cosine distributed
direction with respect to patch normal. Origin and direction are uniquely determined
by the 4-dimensional sample vector xi
*/
RpkRay
Localline::mcrGenerateLocalLine(const Patch *patch, const double *xi) {
    const static Patch *previousPatch = NULL;
    static CoordinateSystem coordSys;
    RpkRay ray;
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
Localline::mcrShootRay(const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, Patch *P, RpkRay *ray, RayHit *hitStore) {
    float distance = Float::MAX_VALUE;
    RayHit *hit;

    // Reject self-intersections
    const RayExclusionSet exclusions(P, P->twin);
    hit = sceneQuery->intersect(
        ray,
        Numeric::EPSILON_FLOAT < P->tolerance ? Numeric::EPSILON_FLOAT : P->tolerance,
        &distance,
        FRONT | POINT,
        hitStore,
        exclusions,
        sceneWorldSceneQueryWorkspace);
#ifdef RPK_RAY_CAPTURE
    if ( sceneWorldSceneQueryWorkspace->getRecorder() != NULL ) {
        sceneWorldSceneQueryWorkspace->getRecorder()->recordQuery(
            VisibilityQueryRecorder::STOCHASTIC_SHOOTING, ray,
            Numeric::EPSILON_FLOAT < P->tolerance ? Numeric::EPSILON_FLOAT : P->tolerance,
            Float::MAX_VALUE, FRONT | POINT, exclusions, hit, distance);
    }
#endif
    someFeedback();

    return hit;
}

#endif
