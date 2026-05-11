#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"
/**
Generic stochastic Jacobi iteration (local lines)
TODO: combined radiance / importance propagation
TODO: hierarchical refinement for importance propagation
TODO: re-incorporate the rejection sampling technique for
sampling positions on shooters with higher order radiosity approximation
(lower variance)
TODO: lines and line bundles.
*/

#include <cstdlib>

#include "vsdk/toolkit/java/lang/System.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Stochjacobi.h"

#ifdef RAYTRACING_ENABLED

#include "vsdk/toolkit/java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/McradP.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Hierarchy.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Ccr.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Localline.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation.h"

StochasticJacobi::GetRadianceCallback StochasticJacobi::getRadianceCallback = nullptr;
StochasticJacobi::GetImportanceCallback StochasticJacobi::getImportanceCallback = nullptr;
StochasticJacobi::UpdateCallback StochasticJacobi::reflectCallback = nullptr;
int StochasticJacobi::useControlVariate = 0; // If uses a constant control variate
int StochasticJacobi::numberOfRaysToShoot = 0; // Number of rays to shoot in the iteration
double StochasticJacobi::sumOfProbabilities = 0.0; // Sum of un-normalised sampling "probabilities"

void
StochasticJacobi::stochasticJacobiInitGlobals(
    int numberOfRays,
    GetRadianceCallback getRadianceCallBack,
    GetImportanceCallback getImportanceCallBack,
    UpdateCallback updateCallBack)
{
    numberOfRaysToShoot = numberOfRays;
    getRadianceCallback = getRadianceCallBack;
    getImportanceCallback = getImportanceCallBack;
    reflectCallback = updateCallBack;
    // Only use control variates for propagating radiance, not for importance
    useControlVariate = (StochasticRelaxation::activeState().constantControlVariate && getRadianceCallBack);

    if ( getRadianceCallback ) {
        StochasticRelaxation::activeState().prevTracedRays = StochasticRelaxation::activeState().tracedRays;
    }
    if ( getImportanceCallback ) {
        StochasticRelaxation::activeState().prevImportanceTracedRays = StochasticRelaxation::activeState().importanceTracedRays;
    }
}

void
StochasticJacobi::stochasticJacobiPrintMessage(long nr_rays) {
    java::System::err.printf("%s-directional ",
            StochasticRelaxation::activeState().bidirectionalTransfers ? "Bi" : "Uni");
    if ( getRadianceCallback && getImportanceCallback ) {
        java::System::err.printf("combined ");
    }
    if ( getRadianceCallback ) {
        java::System::err.printf("%s radiance ",
                StochasticRelaxation::activeState().importanceDriven ? "importance-driven " : "");
    }
    if ( getRadianceCallback && getImportanceCallback ) {
        java::System::err.printf("and ");
    }
    if ( getImportanceCallback ) {
        java::System::err.printf("%s importance ",
                StochasticRelaxation::activeState().radianceDriven ? "radiance-driven " : "");
    }
    java::System::err.printf("propagation");
    if ( useControlVariate ) {
        java::System::err.printf("using a constant control variate ");
    }
    java::System::err.printf("(%ld rays):\n", nr_rays);
}

/**
Compute (un-normalised) stochasticJacobiProbability of shooting a ray from elem
*/
double
StochasticJacobi::stochasticJacobiProbability(const StochasticRadiosityElement *elem) {
    double prob = 0.0;

    if ( getRadianceCallback ) {
        // Probability proportional to power to be propagated
        ColorRgb radiance = getRadianceCallback(elem)[0];
        if ( StochasticRelaxation::activeState().constantControlVariate ) {
            radiance.subtract(radiance, StochasticRelaxation::activeState().controlRadiance);
        }
        prob = elem->area * radiance.sumAbsComponents();
        if ( StochasticRelaxation::activeState().importanceDriven ) {
            // Weight with received importance
            float w = elem->importance - elem->sourceImportance;
            prob *= ((w > 0.0) ? w : 0.0);
        }
    }

    if ( getImportanceCallback && StochasticRelaxation::activeState().importanceDriven ) {
        double prob2 = elem->area * java::Math::abs(getImportanceCallback(elem)) *
                StochasticRadiosityElement::stochasticRadiosityElementScalarReflectance(elem);

        if ( StochasticRelaxation::activeState().radianceDriven ) {
            // Received-radiance weighted importance transport
            ColorRgb receivedRadiance;
            receivedRadiance.subtract(elem->radiance[0], elem->sourceRad);
            prob2 *= receivedRadiance.sumAbsComponents();
        }

        // Equal weight to importance and radiance propagation for constant approximation,
        // but higher weight to radiance for higher order approximations. Still OK
        // if only propagating importance
        prob = prob * StochasticRadiosityBasisState::activeState().approxDesc[StochasticRelaxation::activeState().approximationOrderType].basis_size + prob2;
    }

    return prob;
}

/**
clear accumulators of all kinds of sample weights and contributions
*/
void
StochasticJacobi::stochasticJacobiElementClearAccumulators(StochasticRadiosityElement *elem) {
    if ( getRadianceCallback ) {
        Coefficientsmcrad::stochasticRadiosityClearCoefficients(elem->receivedRadiance, elem->basis);
    }
    if ( getImportanceCallback ) {
        elem->receivedImportance = 0.0;
    }
}

/**
Clears received radiance and importance and accumulates the un-normalized
sampling probabilities at leaf elements
*/
void
StochasticJacobi::stochasticJacobiElementSetup(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = static_cast<StochasticRadiosityElement *>(element);

    if ( stochasticRadiosityElement == nullptr ) {
        return;
    }

    stochasticRadiosityElement->samplingProbability = 0.0;
    if ( !stochasticRadiosityElement->traverseAllChildren(stochasticJacobiElementSetup) ) {
        // Elem is a leaf element. We need to compute the sum of the un-normalized
        // sampling "probabilities" at the leaf elements
        stochasticRadiosityElement->samplingProbability = static_cast<float>(stochasticJacobiProbability(stochasticRadiosityElement));
        sumOfProbabilities += stochasticRadiosityElement->samplingProbability;
    }
    if ( stochasticRadiosityElement->parent ) {
        // The probability of sampling a non-leaf element is the sum of the
        // probabilities of sampling the sub-elements
        static_cast<StochasticRadiosityElement *>(stochasticRadiosityElement->parent)->samplingProbability += stochasticRadiosityElement->samplingProbability;
    }

    stochasticJacobiElementClearAccumulators(stochasticRadiosityElement);
}

/**
Returns true if success, that is: sum of sampling probabilities is nonzero
*/
bool
StochasticJacobi::stochasticJacobiSetup(const java::ArrayList<Patch *> *scenePatches) {
    // Determine constant control radiosity if required
    StochasticRelaxation::activeState().controlRadiance.clear();
    if ( useControlVariate ) {
        StochasticRelaxation::activeState().controlRadiance = Ccr::determineControlRadiosity(getRadianceCallback, nullptr, scenePatches);
    }

    sumOfProbabilities = 0.0;
    stochasticJacobiElementSetup(ElementHierarchyState::activeState().topCluster);

    if ( sumOfProbabilities < Numeric::EPSILON * Numeric::EPSILON ) {
        Logger::warning("Iteration", "No sources");
        return false;
    }
    return true;
}

/**
Returns radiance to be propagated from the given location of the element
*/
ColorRgb
StochasticJacobi::stochasticJacobiGetSourceRadiance(const StochasticRadiosityElement *src, double us, double vs) {
    const ColorRgb *srcRad = getRadianceCallback(src);
    return Basismcrad::colorAtUv(src->basis, srcRad, us, vs);
}

void
StochasticJacobi::stochasticJacobiPropagateRadianceToSurface(
    StochasticRadiosityElement *rcv,
    double ur,
    double vr,
    ColorRgb rayPower,
    const StochasticRadiosityElement * /*src*/,
    double fraction,
    double /*weight*/)
{
    for ( int i = 0; i < rcv->basis->size; i++ ) {
        double dual = rcv->basis->dualFunction[i](ur, vr) / rcv->area;
        double w = dual * fraction / static_cast<double>(numberOfRaysToShoot);
        rcv->receivedRadiance[i].addScaled(rcv->receivedRadiance[i], static_cast<float>(w), rayPower);
    }
}

void
StochasticJacobi::stochasticJacobiPropagateRadianceToClusterIsotropic(
    StochasticRadiosityElement *cluster,
    ColorRgb rayPower,
    const StochasticRadiosityElement * /*src*/,
    double fraction,
    double /*weight*/)
{
    double w = fraction / cluster->area / static_cast<double>(numberOfRaysToShoot);
    cluster->receivedRadiance[0].addScaled(cluster->receivedRadiance[0], static_cast<float>(w), rayPower);
}

/**
Note: Not considering the MAX_HIERARCHY_DEPTH limit.
*/
void
StochasticJacobi::stochasticJacobiPropagateRadianceClusterRecursive(
    StochasticRadiosityElement *currentElement,
    ColorRgb rayPower,
    Ray *ray,
    float dir,
    double projectedArea,
    double fraction)
{
    if ( currentElement != nullptr && !currentElement->isCluster() ) {
        // Trivial case
        double c = -dir * currentElement->patch->getNormal().dotProduct(ray->direction);
        if ( c > 0.0 ) {
            double aFraction = fraction * (c * currentElement->area / projectedArea);
            double w = aFraction / currentElement->area / static_cast<double>(numberOfRaysToShoot);
            currentElement->receivedRadiance[0].addScaled(currentElement->receivedRadiance[0], static_cast<float>(w), rayPower);
        }
    } else {
        // Recursive case
        for ( int i = 0; currentElement != nullptr && currentElement->irregularSubElements != nullptr && i < currentElement->irregularSubElements->size(); i++ ) {
            stochasticJacobiPropagateRadianceClusterRecursive(
                static_cast<StochasticRadiosityElement *>(currentElement->irregularSubElements->get(i)),
                rayPower,
                ray,
                dir,
                projectedArea,
                fraction);
        }
    }
}

void
StochasticJacobi::stochasticJacobiPropagateRadianceToClusterOriented(
    StochasticRadiosityElement *cluster,
    ColorRgb rayPower,
    Ray *ray,
    float dir,
    const StochasticRadiosityElement * /*src*/,
    double projectedArea,
    double fraction,
    double /*weight*/)
{
    stochasticJacobiPropagateRadianceClusterRecursive(cluster, rayPower, ray, dir, projectedArea, fraction);
}

/**
Note: Not considering the MAX_HIERARCHY_DEPTH limit.
*/
void
StochasticJacobi::stochasticJacobiReceiverProjectedAreaRecursive(
    const StochasticRadiosityElement *currentElement,
    Ray *ray,
    float dir,
    double *area)
{
    if ( currentElement != nullptr && !currentElement->isCluster() ) {
        // Trivial case
        double c = -dir * currentElement->patch->getNormal().dotProduct(ray->direction);
        if ( c > 0.0 ) {
            *area += c * currentElement->area;
        }
    } else {
        // Recursive case
        for ( int i = 0; currentElement != nullptr && currentElement->irregularSubElements != nullptr &&
                 i < currentElement->irregularSubElements->size(); i++ ) {
            stochasticJacobiReceiverProjectedAreaRecursive(
                static_cast<StochasticRadiosityElement *>(currentElement->irregularSubElements->get(i)),
                ray,
                dir,
                area);
        }
    }
}

double
StochasticJacobi::stochasticJacobiReceiverProjectedArea(const StochasticRadiosityElement *cluster, Ray *ray, float dir) {
    double area = 0.0;
    stochasticJacobiReceiverProjectedAreaRecursive(cluster, ray, dir, &area);
    return area;
}

/**
Transfer radiance from src to rcv.
src_prob = un-normalised src birth stochasticJacobiProbability / src area
rcv_prob = un-normalised rcv birth stochasticJacobiProbability / rcv area for bidirectional transfers
      or = 0 for unidirectional transfers
score is weighted with sumOfProbabilities / numberOfRaysToShoot.
ray->dir and dir are used in order to determine projected cluster area
and cosine of incident direction of cluster surface elements when
the receiver is a cluster
*/
void
StochasticJacobi::stochasticJacobiPropagateRadiance(
    const StochasticRadiosityElement *src,
    double us,
    double vs,
    StochasticRadiosityElement *rcv,
    double ur,
    double vr,
    double src_prob,
    double rcv_prob,
    Ray *ray,
    float dir)
{
    ColorRgb radiance;
    ColorRgb rayPower;
    double area;
    double weight = sumOfProbabilities / src_prob; // src area / normalised src prob
    double fraction = src_prob / (src_prob + rcv_prob); // 1 for uni-directional transfers

    if ( src_prob < Numeric::EPSILON * Numeric::EPSILON /* this should never happen */
         || fraction < Numeric::EPSILON ) {
        // Reverse transfer from a black surface
        return;
    }

    radiance = stochasticJacobiGetSourceRadiance(src, us, vs);
    if ( StochasticRelaxation::activeState().constantControlVariate ) {
        radiance.subtract(radiance, StochasticRelaxation::activeState().controlRadiance);
    }
    rayPower.scaledCopy(static_cast<float>(weight), radiance);

    if ( !rcv->isCluster() ) {
        stochasticJacobiPropagateRadianceToSurface(rcv, ur, vr, rayPower, src, fraction, weight);
    } else {
        switch ( ElementHierarchyState::activeState().clustering ) {
            case HierarchyClusteringMode::NO_CLUSTERING:
                Logger::fatal(-1, "Propagate", "Hierarchy::hierarchyRefine() returns cluster although clustering is disabled.\n");

            case HierarchyClusteringMode::ISOTROPIC_CLUSTERING:
                stochasticJacobiPropagateRadianceToClusterIsotropic(rcv, rayPower, src, fraction, weight);
                break;
            case HierarchyClusteringMode::ORIENTED_CLUSTERING:
                area = stochasticJacobiReceiverProjectedArea(rcv, ray, dir);
                if ( area > Numeric::EPSILON ) {
                    stochasticJacobiPropagateRadianceToClusterOriented(rcv, rayPower, ray, dir, src, area, fraction,
                                                                       weight);
                }
                break;
            default:
                Logger::fatal(-1, "Propagate", "Invalid clustering mode %d\n", static_cast<int>(ElementHierarchyState::activeState().clustering));
        }
    }
}

/**
Idem but for importance
*/
void
StochasticJacobi::stochasticJacobiPropagateImportance(
    const StochasticRadiosityElement *src,
    double /*us*/,
    double /*vs*/,
    StochasticRadiosityElement *rcv,
    double /*ur*/,
    double /*vr*/,
    double src_prob,
    double rcv_prob,
    const Ray * /*ray*/,
    float /*dir*/)
{
    double w = sumOfProbabilities / (src_prob + rcv_prob) / rcv->area / static_cast<double>(numberOfRaysToShoot);
    rcv->receivedImportance += static_cast<float>(w * StochasticRadiosityElement::stochasticRadiosityElementScalarReflectance(src) * getImportanceCallback(src));

    if ( ElementHierarchyState::activeState().do_h_meshing ||
         ElementHierarchyState::activeState().clustering != HierarchyClusteringMode::NO_CLUSTERING ) {
        Logger::fatal(-1, "Propagate", "Importance propagation not implemented in combination with hierarchical refinement");
    }
}

/**
Src is the leaf element containing the point from which to propagate
radiance on P. P and Q are toplevel surface elements. Transfer
is from P to Q
*/
void
StochasticJacobi::stochasticJacobiRefineAndPropagateRadiance(
    const StochasticRadiosityElement *src,
    double us,
    double vs,
    StochasticRadiosityElement *P,
    double up,
    double vp,
    StochasticRadiosityElement *Q,
    double uq,
    double vq,
    double src_prob,
    double rcv_prob,
    Ray *ray,
    float dir,
    const RendererConfiguration *renderOptions)
{
    Link link{};
    link = Hierarchy::topLink(Q, P);
    Hierarchy::hierarchyRefine(&link, Q, &uq, &vq, P, &up, &vp, ElementHierarchyState::activeState().oracle, renderOptions);
    // Propagate from the leaf element src to the admissible receiver element containing/contained by Q
    stochasticJacobiPropagateRadiance(src, us, vs, link.rcv, uq, vq, src_prob, rcv_prob, ray, dir);
}

void
StochasticJacobi::stochasticJacobiRefineAndPropagateImportance(
    const StochasticRadiosityElement *P,
    double up,
    double vp,
    StochasticRadiosityElement *Q,
    double uq,
    double vq,
    double src_prob,
    double rcv_prob,
    const Ray *ray,
    float dir)
{
    // No refinement (yet) for importance: propagate between toplevel surfaces
    stochasticJacobiPropagateImportance(P, up, vp, Q, uq, vq, src_prob, rcv_prob, ray, dir);
}

/**
Ray is a ray connecting the positions with given (u,v) parameters
on the toplevel surface element P to Q. This routine refines the
imaginary interaction between these elements and performs
radiance or importance transfer along the ray, taking into account
bi-directionality if requested
*/
void
StochasticJacobi::stochasticJacobiRefineAndPropagate(
    StochasticRadiosityElement *P,
    double up,
    double vp,
    StochasticRadiosityElement *Q,
    double uq,
    double vq,
    Ray *ray,
    const RendererConfiguration *renderOptions)
{
    double src_prob;
    double us = up;
    double vs = vp;
    const StochasticRadiosityElement *src = StochasticRadiosityElement::stochasticRadiosityElementRegularLeafElementAtPoint(P, &us, &vs);
    src_prob = static_cast<double>(src->samplingProbability) / static_cast<double>(src->area);
    if ( StochasticRelaxation::activeState().bidirectionalTransfers ) {
        double rcv_prob;
        double ur = uq;
        double vr = vq;
        const StochasticRadiosityElement *rcv = StochasticRadiosityElement::stochasticRadiosityElementRegularLeafElementAtPoint(Q, &ur, &vr);
        rcv_prob = static_cast<double>(rcv->samplingProbability) / static_cast<double>(rcv->area);

        if ( getRadianceCallback ) {
            stochasticJacobiRefineAndPropagateRadiance(src, us, vs, P, up, vp, Q, uq, vq, src_prob, rcv_prob, ray, +1, renderOptions);
            stochasticJacobiRefineAndPropagateRadiance(rcv, ur, vr, Q, uq, vq, P, up, vp, rcv_prob, src_prob, ray, -1, renderOptions);
        }
        if ( getImportanceCallback ) {
            stochasticJacobiRefineAndPropagateImportance(P, up, vp, Q, uq, vq, src_prob, rcv_prob, ray, +1);
            stochasticJacobiRefineAndPropagateImportance(Q, uq, vq, P, up, vp, rcv_prob, src_prob, ray, -1);
        }
    } else {
        if ( getRadianceCallback ) {
            stochasticJacobiRefineAndPropagateRadiance(src, us, vs, P, up, vp, Q, uq, vq, src_prob, 0.0, ray, +1, renderOptions);
        }
        if ( getImportanceCallback ) {
            stochasticJacobiRefineAndPropagateImportance(P, up, vp, Q, uq, vq, src_prob, 0.0, ray, +1);
        }
    }
}

double *
StochasticJacobi::stochasticJacobiNextSample(
    StochasticRadiosityElement *elem,
    int nMostSignificantBit,
    NiederreiterIndex mostSignificantBit1,
    NiederreiterIndex rMostSignificantBit2,
    double *zeta)
{
    NiederreiterIndex *xi;
    NiederreiterIndex u;
    NiederreiterIndex v;
    // Use different ray index for propagating importance and radiance
    NiederreiterIndex *ray_index = getRadianceCallback ? &elem->rayIndex : &elem->importanceRayIndex;

    xi = Niederreiter::NextNiedInRange(ray_index, +1, nMostSignificantBit, mostSignificantBit1, rMostSignificantBit2);

    (*ray_index)++;
    u = (xi[0] & ~3) | 1; // Avoid positions on sub-element boundaries
    v = (xi[1] & ~3) | 1;
    if ( elem->numberOfVertices == 3 ) {
        Niederreiter::foldSample(&u, &v);
    }
    zeta[0] = static_cast<double>(u) * Niederreiter::RECIP;
    zeta[1] = static_cast<double>(v) * Niederreiter::RECIP;
    zeta[2] = static_cast<double>(xi[2]) * Niederreiter::RECIP;
    zeta[3] = static_cast<double>(xi[3]) * Niederreiter::RECIP;
    return zeta;
}

/**
Determines uniform (u,v) parameters of hit point on hit patch
*/
void
StochasticJacobi::stochasticJacobiUniformHitCoordinates(const RayHit *hit, double *uHit, double *vHit) {
    if ( hit->getFlags() & RayHitFlag::UV ) {
        // (u,v) coordinates obtained as side result of intersection test
        *uHit = hit->getUv().u;
        *vHit = hit->getUv().v;
        if ( hit->getPatch()->getJacobian() != nullptr ) {
            hit->getPatch()->biLinearToUniform(uHit, vHit);
        }
    } else {
        Vector3D position = hit->getPoint();
        hit->getPatch()->uniformUv(&position, uHit, vHit);
    }

    // Clip uv coordinates to lay strictly inside the hit patch
    if ( *uHit < Numeric::EPSILON ) {
        *uHit = Numeric::EPSILON;
    }
    if ( *vHit < Numeric::EPSILON ) {
        *vHit = Numeric::EPSILON;
    }
    if ( *uHit > 1.0 - Numeric::EPSILON ) {
        *uHit = 1.0 - Numeric::EPSILON;
    }
    if ( *vHit > 1.0 - Numeric::EPSILON ) {
        *vHit = 1.0 - Numeric::EPSILON;
    }
}

/**
Traces a local line from 'src' and propagates radiance and/or importance from P to
hit patch (and back for bidirectional transfers)
*/
void
StochasticJacobi::stochasticJacobiElementShootRay(
    const VoxelGrid * sceneWorldVoxelGrid,
    StochasticRadiosityElement *src,
    int nMostSignificantBit,
    NiederreiterIndex mostSignificantBit1,
    NiederreiterIndex rMostSignificantBit2,
    const RendererConfiguration *renderOptions)
{
    if ( getRadianceCallback != nullptr ) {
        StochasticRelaxation::activeState().tracedRays++;
    }

    if ( getImportanceCallback != nullptr ) {
        StochasticRelaxation::activeState().importanceTracedRays++;
    }

    double zeta[4];
    Ray ray = Localline::mcrGenerateLocalLine(src->patch,
                               stochasticJacobiNextSample(src, nMostSignificantBit, mostSignificantBit1, rMostSignificantBit2, zeta));

    RayHit hitStore;
    const RayHit *hit = Localline::mcrShootRay(sceneWorldVoxelGrid, src->patch, &ray, &hitStore);

    if ( hit ) {
        double uHit = 0.0;
        double vHit = 0.0;
        stochasticJacobiUniformHitCoordinates(hit, &uHit, &vHit);
        stochasticJacobiRefineAndPropagate(McradP::topLevelStochasticRadiosityElement(src->patch), zeta[0], zeta[1],
                                           McradP::topLevelStochasticRadiosityElement(hit->getPatch()), uHit, vHit, &ray, renderOptions);
    } else {
        StochasticRelaxation::activeState().numberOfMisses++;
    }
}

void
StochasticJacobi::stochasticJacobiInitPushRayIndex(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = static_cast<StochasticRadiosityElement *>(element);

    if ( stochasticRadiosityElement == nullptr ) {
        return;
    }
    stochasticRadiosityElement->rayIndex = static_cast<StochasticRadiosityElement *>(stochasticRadiosityElement->parent)->rayIndex;
    stochasticRadiosityElement->importanceRayIndex = static_cast<StochasticRadiosityElement *>(stochasticRadiosityElement->parent)->importanceRayIndex;
    stochasticRadiosityElement->traverseAllChildren(stochasticJacobiInitPushRayIndex);
}

/**
Determines nr of rays to shoot from element and shoots this number of rays
*/
void
StochasticJacobi::stochasticJacobiElementShootRays(
    const VoxelGrid *sceneWorldVoxelGrid,
    StochasticRadiosityElement *element,
    int raysThisElem,
    const RendererConfiguration *renderOptions)
{
    int sampleRange; // Determines a range in which to generate a sample
    NiederreiterIndex mostSignificantBit1; // See monteCarloRadiosityElementRange() and NextSample()
    NiederreiterIndex rMostSignificantBit2;

    // Sample number range for 4D Niederreiter sequence
    StochasticRadiosityElement::stochasticRadiosityElementRange(element, &sampleRange, &mostSignificantBit1, &rMostSignificantBit2);

    // Shoot the rays
    for ( int i = 0; i < raysThisElem; i++ ) {
        stochasticJacobiElementShootRay(sceneWorldVoxelGrid, element, sampleRange, mostSignificantBit1, rMostSignificantBit2, renderOptions);
    }

    if ( element != nullptr && !element->isLeaf() ) {
        // Source got subdivided while shooting the rays
        element->traverseAllChildren(stochasticJacobiInitPushRayIndex);
    }
}

void
StochasticJacobi::stochasticJacobiShootRaysRecursive(
    VoxelGrid *sceneWorldVoxelGrid,
    StochasticRadiosityElement *element,
    double rnd,
    long *rayCount,
    double *cumulative,
    RendererConfiguration *renderOptions) {
    if ( element->regularSubElements == nullptr ) {
        // Trivial case
        double p = element->samplingProbability / sumOfProbabilities;
        long rays_this_leaf =
                static_cast<long>(java::Math::floor((*cumulative + p) * static_cast<double>(numberOfRaysToShoot) + rnd)) - *rayCount;

        if ( rays_this_leaf > 0 ) {
            stochasticJacobiElementShootRays(sceneWorldVoxelGrid, element, static_cast<int>(rays_this_leaf), renderOptions);
        }

        *cumulative += p;
        *rayCount += rays_this_leaf;
    } else {
        // Recursive case
        for ( int i = 0; i < 4; i++ ) {
            stochasticJacobiShootRaysRecursive(
                sceneWorldVoxelGrid,
                static_cast<StochasticRadiosityElement *>(element->regularSubElements[i]),
                rnd,
                rayCount,
                cumulative,
                renderOptions);
        }
    }
}

/**
Fire off rays from the leaf elements, propagate radiance/importance
*/
void
StochasticJacobi::stochasticJacobiShootRays(
    VoxelGrid *sceneWorldVoxelGrid,
    const java::ArrayList<Patch *> *scenePatches,
    RendererConfiguration *renderOptions)
{
    double rnd = drand48();
    long rayCount = 0;
    double cumulative = 0.0;

    // Loop over all leaf elements in the element hierarchy
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        stochasticJacobiShootRaysRecursive(
            sceneWorldVoxelGrid,
            McradP::topLevelStochasticRadiosityElement(scenePatches->get(i)),
            rnd,
            &rayCount,
            &cumulative,
            renderOptions);
    }

    java::System::err.printf("\n");
}

/**
Converts received radiance and importance at a leaf element into a new
approximation of total and un-shot radiance and importance
*/
void
StochasticJacobi::stochasticJacobiUpdateElement(StochasticRadiosityElement *elem) {
    if ( getRadianceCallback ) {
        if ( useControlVariate ) {
            // Add constant radiosity contribution to received flux
            elem->receivedRadiance[0].add(
                elem->receivedRadiance[0], StochasticRelaxation::activeState().controlRadiance);
        }
        // Multiply with reflectivity on leaf elements only
        Coefficientsmcrad::stochasticRadiosityMultiplyCoefficients(elem->Rd, elem->receivedRadiance, elem->basis);
    }

    reflectCallback(elem, static_cast<double>(numberOfRaysToShoot) / sumOfProbabilities);

    StochasticRelaxation::activeState().unShotFlux.addScaled(
        StochasticRelaxation::activeState().unShotFlux,
        static_cast<float>(M_PI) * elem->area,
        elem->unShotRadiance[0]);
    StochasticRelaxation::activeState().totalFlux.addScaled(
        StochasticRelaxation::activeState().totalFlux,
        static_cast<float>(M_PI) * elem->area,
        elem->radiance[0]);
    StochasticRelaxation::activeState().indirectImportanceWeightedUnShotFlux.addScaled(
        StochasticRelaxation::activeState().indirectImportanceWeightedUnShotFlux,
        static_cast<float>(M_PI) * elem->area * (elem->importance - elem->sourceImportance),
        elem->unShotRadiance[0]);
    StochasticRelaxation::activeState().unShotYmp += (elem->area * java::Math::abs(elem->unShotImportance));
    StochasticRelaxation::activeState().totalYmp += elem->area * elem->importance;
}

void
StochasticJacobi::stochasticJacobiPush(const StochasticRadiosityElement *parent, StochasticRadiosityElement *child) {
    if ( getRadianceCallback ) {
        ColorRgb Rd;
        Rd.clear();

        if ( parent->isCluster() && !child->isCluster() ) {
            // Multiply with reflectance (See PropagateRadianceToClusterIsotropic() above)
            ColorRgb rad = parent->receivedRadiance[0];
            Rd = child->Rd;
            rad.selfScalarProduct(Rd);
            StochasticRadiosityElement::stochasticRadiosityElementPushRadiance(parent, child, &rad, child->receivedRadiance);
        } else
            StochasticRadiosityElement::stochasticRadiosityElementPushRadiance(parent, child, parent->receivedRadiance, child->receivedRadiance);
    }

    if ( getImportanceCallback ) {
        StochasticRadiosityElement::stochasticRadiosityElementPushImportance(&parent->receivedImportance, &child->receivedImportance);
    }
}

void
StochasticJacobi::stochasticJacobiPull(StochasticRadiosityElement *parent, const StochasticRadiosityElement *child) {
    if ( getRadianceCallback ) {
        StochasticRadiosityElement::stochasticRadiosityElementPullRadiance(parent, child, parent->radiance, child->radiance);
        StochasticRadiosityElement::stochasticRadiosityElementPullRadiance(parent, child, parent->unShotRadiance, child->unShotRadiance);
    }
    if ( getImportanceCallback ) {
        StochasticRadiosityElement::stochasticRadiosityElementPullImportance(parent, child, &parent->importance, &child->importance);
        StochasticRadiosityElement::stochasticRadiosityElementPullImportance(parent, child, &parent->unShotImportance, &child->unShotImportance);
    }
}

/**
Clears everything to be pulled from children elements to zero
*/
void
StochasticJacobi::stochasticJacobiClearElement(StochasticRadiosityElement *parent) {
    if ( getRadianceCallback ) {
        Coefficientsmcrad::stochasticRadiosityClearCoefficients(parent->radiance, parent->basis);
        Coefficientsmcrad::stochasticRadiosityClearCoefficients(parent->unShotRadiance, parent->basis);
    }
    if ( getImportanceCallback ) {
        parent->importance = parent->unShotImportance = 0.0;
    }
}

void
StochasticJacobi::stochasticJacobiPushUpdatePullChild(Element *element) {
    StochasticRadiosityElement *child = static_cast<StochasticRadiosityElement *>(element);
    StochasticRadiosityElement *parent = static_cast<StochasticRadiosityElement *>(child->parent);
    stochasticJacobiPush(parent, child);
    stochasticJacobiPushUpdatePull(child);
    stochasticJacobiPull(parent, child);
}

void
StochasticJacobi::stochasticJacobiPushUpdatePull(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = static_cast<StochasticRadiosityElement *>(element);
    if ( stochasticRadiosityElement != nullptr && stochasticRadiosityElement->isLeaf() ) {
        stochasticJacobiUpdateElement(stochasticRadiosityElement);
    } else if ( element != nullptr ) {
        // Not a leaf element
        stochasticJacobiClearElement(stochasticRadiosityElement);
        element->traverseAllChildren(stochasticJacobiPushUpdatePullChild);
    }
}

void
StochasticJacobi::stochasticJacobiPullRdEdFromChild(Element *element) {
    StochasticRadiosityElement *child = static_cast<StochasticRadiosityElement *>(element);
    StochasticRadiosityElement *parent = static_cast<StochasticRadiosityElement *>(child->parent);

    stochasticJacobiPullRdEd(child);

    parent->Ed.addScaled(parent->Ed, child->area / parent->area, child->Ed);
    parent->Rd.addScaled(parent->Rd, child->area / parent->area, child->Rd);
    if ( parent->isCluster() )
        parent->Rd.setMonochrome(1.0);
}

void
StochasticJacobi::stochasticJacobiPullRdEd(StochasticRadiosityElement *element) {
    if ( element == nullptr || element->isLeaf() || (!element->isCluster() && !StochasticRadiosityElement::stochasticRadiosityElementIsTextured(element)) ) {
        return;
    }

    element->Ed.clear();
    element->Rd.clear();
    element->traverseAllChildren(stochasticJacobiPullRdEdFromChild);
}

void
StochasticJacobi::stochasticJacobiPushUpdatePullSweep() {
    // Update radiance, compute new total and un-shot flux
    StochasticRelaxation::activeState().unShotFlux.clear();
    StochasticRelaxation::activeState().unShotYmp = 0.0;
    StochasticRelaxation::activeState().totalFlux.clear();
    StochasticRelaxation::activeState().totalYmp = 0.0;
    StochasticRelaxation::activeState().indirectImportanceWeightedUnShotFlux.clear();

    // Update reflectances and emittances (refinement yields more accurate estimates
    // on textured surfaces)
    stochasticJacobiPullRdEd(ElementHierarchyState::activeState().topCluster);

    stochasticJacobiPushUpdatePull(ElementHierarchyState::activeState().topCluster);
}

/**
Generic routine for Stochastic Jacobi iterations:
- nr_rays: nr of rays to use
- getRadianceCallBack: routine returning radiance (total or un-shot) to be
propagated for a given element, or nullptr if no radiance propagation is
required.
- getImportanceCallBack: same, but for importance.
- updateCallBack: routine updating total, un-shot and source radiance and/or
importance based on result received during the iteration.

The operation of this routine is further controlled by stochastic relaxation
state parameters:
- constantControlVariate: perform constant control variate variance reduction
- bidirectionalTransfers: use lines bidirectionally
- importanceDriven: importance-driven radiance propagation
- radianceDriven: radiance-driven importance propagation
- hierarchy.do_h_meshing, hierarchy.clustering: hierarchical refinement/clustering

This routine updates global ray counts and total/un-shot power/importance statistics.

CAVEAT: propagate either radiance or importance alone. Simultaneous
propagation of importance and radiance does not work yet.
*/
void
StochasticJacobi::doStochasticJacobiIteration(
    VoxelGrid *sceneWorldVoxelGrid,
    long numberOfRays,
    GetRadianceCallback getRadianceCallBack,
    GetImportanceCallback getImportanceCallBack,
    UpdateCallback updateCallBack,
    const java::ArrayList<Patch *> *scenePatches,
    RendererConfiguration *renderOptions)
{
    stochasticJacobiInitGlobals(static_cast<int>(numberOfRays), getRadianceCallBack, getImportanceCallBack, updateCallBack);
    stochasticJacobiPrintMessage(numberOfRays);
    if ( !stochasticJacobiSetup(scenePatches) ) {
        return;
    }
    stochasticJacobiShootRays(sceneWorldVoxelGrid, scenePatches, renderOptions);
    stochasticJacobiPushUpdatePullSweep();
}

#endif
