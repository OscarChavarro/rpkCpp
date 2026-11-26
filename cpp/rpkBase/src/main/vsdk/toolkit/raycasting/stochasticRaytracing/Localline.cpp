#include "java/lang/Double.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"
/**
Generate and trace a local line
*/
#include "java/lang/System.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "vsdk/toolkit/common/linealAlgebra/CoordinateSystem.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/McradP.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Localline.h"

#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation.h"

/**
Creates a coordinate system on the patch P with Z direction along the normal
*/
void
Localline::patchCoordSys(const Patch *patch, CoordinateSystem *coord) {
    Vector3Dd z = patch->getNormal();
    Vector3Dd x;
    x = (*patch->getVertices()[1]->point).subtract(*patch->getVertices()[0]->point);
    x = x.normalized(Numeric::EPSILON);
    Vector3Dd y;
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
Ray
Localline::mcrGenerateLocalLine(const Patch *patch, const double *xi) {
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

    Vector3Dd origin;
    patch->uniformPoint(xi[0], xi[1], &origin);
    ray.setOrigin(origin);
    // Section [ARVO1995b].2: use two uniform samples from [0,1]^2 to
    // generate a local hemisphere direction in the patch frame.
    ray.setDirection(coordSys.sampleHemisphereCosTheta(xi[2], xi[3], &pdf));

    return ray;
}

/**
In order to let the user have the impression that the computations are proceeding
*/
void
Localline::someFeedback() {
    if ( (StochasticRelaxation::activeState().tracedRays + StochasticRelaxation::activeState().importanceTracedRays) % 1000 == 0 ) {
        java::System::err.print(".");
    }
}

/**
Determines nearest intersection point and patch
*/
RpkRayHit *
Localline::mcrShootRay(const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, const Patch *P, Ray *ray, RpkRayHit *hitStore) {
    double distance = java::Double::MAX_VALUE;
    RpkRayHit *hit;

    // Reject self-intersections
    const RayExclusionSet exclusions(P, P->getTwin());
    hit = sceneQuery->intersect(
        ray,
        Numeric::EPSILON < P->getTolerance() ? Numeric::EPSILON : P->getTolerance(),
        &distance,
        RayHitFlag::FRONT | RayHitFlag::POINT,
        hitStore,
        exclusions,
        sceneWorldSceneQueryWorkspace);
    someFeedback();

    return hit;
}

#endif
