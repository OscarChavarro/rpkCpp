/**
Generic stochastic Jacobi iteration (local lines)
TODO: combined radiance / importance propagation
TODO: hierarchical refinement for importance propagation
TODO: re-incorporate the rejection sampling technique for
sampling positions on shooters with higher order radiosity approximation
(lower variance)
TODO: global lines and global line bundles.
*/

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "raycasting/stochasticRaytracing/ccr.h"
#include "raycasting/stochasticRaytracing/localline.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

// Returns radiance or importance to be propagated
static ColorRgb *(*globalGetRadianceCallback)(StochasticRadiosityElement *);

static float (*globalGetImportanceCallback)(StochasticRadiosityElement *);

// Converts received radiance or importance into a new approximation for
// total and un-shot radiance or importance
static void (*globalReflectCallback)(StochasticRadiosityElement *, double) = nullptr;

static int globalDoControlVariate; // If uses a constant control variate
static int globalNumberOfRays; // Number of rays to shoot in the iteration
static double globalSumOfProbabilities = 0.0; // Sum of un-normalised sampling "probabilities"

static void
stochasticJacobiInitGlobals(
    int numberOfRays,
    ColorRgb *(*getRadianceCallBack)(StochasticRadiosityElement *),
    float (*GetImportance)(StochasticRadiosityElement *),
    void (*Update)(StochasticRadiosityElement *P, double w))
{
    globalNumberOfRays = numberOfRays;
    globalGetRadianceCallback = getRadianceCallBack;
    globalGetImportanceCallback = GetImportance;
    globalReflectCallback = Update;
    // Only use control variates for propagating radiance, not for importance
    globalDoControlVariate = (GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate && getRadianceCallBack);

    if ( globalGetRadianceCallback ) {
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevTracedRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays;
    }
    if ( globalGetImportanceCallback ) {
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevImportanceTracedRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays;
    }
}

static void
stochasticJacobiPrintMessage(long nr_rays) {
    fprintf(stderr, "%s-directional ",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.bidirectionalTransfers ? "Bi" : "Uni");
    if ( globalGetRadianceCallback && globalGetImportanceCallback ) {
        fprintf(stderr, "combined ");
    }
    if ( globalGetRadianceCallback ) {
        fprintf(stderr, "%s radiance ",
                GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ? "importance-driven " : "");
    }
    if ( globalGetRadianceCallback && globalGetImportanceCallback ) {
        fprintf(stderr, "and ");
    }
    if ( globalGetImportanceCallback ) {
        fprintf(stderr, "%s importance ",
                GLOBAL_stochasticRaytracing_monteCarloRadiosityState.radianceDriven ? "radiance-driven " : "");
    }
    fprintf(stderr, "propagation");
    if ( globalDoControlVariate ) {
        fprintf(stderr, "using a constant control variate ");
    }
    fprintf(stderr, "(%ld rays):\n", nr_rays);
}

/**
Compute (un-normalised) stochasticJacobiProbability of shooting a ray from elem
*/
static double
stochasticJacobiProbability(StochasticRadiosityElement *elem) {
    double prob = 0.0;

    if ( globalGetRadianceCallback ) {
        // Probability proportional to power to be propagated
        ColorRgb radiance = globalGetRadianceCallback(elem)[0];
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate ) {
            radiance.subtract(radiance, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance);
        }
        prob = elem->area * radiance.sumAbsComponents();
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
            // Weight with received importance
            float w = elem->importance - elem->sourceImportance;
            prob *= ((w > 0.0) ? w : 0.0);
        }
    }

    if ( globalGetImportanceCallback && GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
        double prob2 = elem->area * std::fabs(globalGetImportanceCallback(elem)) *
                stochasticRadiosityElementScalarReflectance(elem);

        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.radianceDriven ) {
            // Received-radiance weighted importance transport
            ColorRgb receivedRadiance;
            receivedRadiance.subtract(elem->radiance[0], elem->sourceRad);
            prob2 *= receivedRadiance.sumAbsComponents();
        }

        // Equal weight to importance and radiance propagation for constant approximation,
        // but higher weight to radiance for higher order approximations. Still OK
        // if only propagating importance
        prob = prob * GLOBAL_stochasticRadiosity_approxDesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size + prob2;
    }

    return prob;
}

/**
clear accumulators of all kinds of sample weights and contributions
*/
static void
stochasticJacobiElementClearAccumulators(StochasticRadiosityElement *elem) {
    if ( globalGetRadianceCallback ) {
        stochasticRadiosityClearCoefficients(elem->receivedRadiance, elem->basis);
    }
    if ( globalGetImportanceCallback ) {
        elem->receivedImportance = 0.0;
    }
}

/**
Clears received radiance and importance and accumulates the un-normalized
sampling probabilities at leaf elements
*/
static void
stochasticJacobiElementSetup(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = (StochasticRadiosityElement *)element;

    if ( stochasticRadiosityElement == nullptr ) {
        return;
    }

    stochasticRadiosityElement->samplingProbability = 0.0;
    if ( !stochasticRadiosityElement->traverseAllChildren(stochasticJacobiElementSetup) ) {
        // Elem is a leaf element. We need to compute the sum of the un-normalized
        // sampling "probabilities" at the leaf elements
        stochasticRadiosityElement->samplingProbability = (float)stochasticJacobiProbability(stochasticRadiosityElement);
        globalSumOfProbabilities += stochasticRadiosityElement->samplingProbability;
    }
    if ( stochasticRadiosityElement->parent ) {
        // The probability of sampling a non-leaf element is the sum of the
        // probabilities of sampling the sub-elements
        ((StochasticRadiosityElement *)stochasticRadiosityElement->parent)->samplingProbability += stochasticRadiosityElement->samplingProbability;
    }

    stochasticJacobiElementClearAccumulators(stochasticRadiosityElement);
}

/**
Returns true if success, that is: sum of sampling probabilities is nonzero
*/
static int
stochasticJacobiSetup(java::ArrayList<Patch *> *scenePatches) {
    // Determine constant control radiosity if required
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance.clear();
    if ( globalDoControlVariate ) {
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance = determineControlRadiosity(globalGetRadianceCallback, nullptr, scenePatches);
    }

    globalSumOfProbabilities = 0.0;
    stochasticJacobiElementSetup(GLOBAL_stochasticRaytracing_hierarchy.topCluster);

    if ( globalSumOfProbabilities < EPSILON * EPSILON ) {
        logWarning("Iteration", "No sources");
        return false;
    }
    return true;
}

/**
Returns radiance to be propagated from the given location of the element
*/
static ColorRgb
stochasticJacobiGetSourceRadiance(StochasticRadiosityElement *src, double us, double vs) {
    ColorRgb *srcRad = globalGetRadianceCallback(src);
    return colorAtUv(src->basis, srcRad, us, vs);
}

static void
stochasticJacobiPropagateRadianceToSurface(
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
        double w = dual * fraction / (double) globalNumberOfRays;
        rcv->receivedRadiance[i].addScaled(rcv->receivedRadiance[i], (float) w, rayPower);
    }
}

static void
stochasticJacobiPropagateRadianceToClusterIsotropic(
    StochasticRadiosityElement *cluster,
    ColorRgb rayPower,
    const StochasticRadiosityElement * /*src*/,
    double fraction,
    double /*weight*/)
{
    double w = fraction / cluster->area / (double) globalNumberOfRays;
    cluster->receivedRadiance[0].addScaled(cluster->receivedRadiance[0], (float) w, rayPower);
}

/**
Note: Not considering the MAX_HIERARCHY_DEPTH limit.
*/
static void
stochasticJacobiPropagateRadianceClusterRecursive(
    StochasticRadiosityElement *currentElement,
    ColorRgb rayPower,
    Ray *ray,
    float dir,
    double projectedArea,
    double fraction)
{
    if ( currentElement != nullptr && !currentElement->isCluster() ) {
        // Trivial case
        double c = -dir * currentElement->patch->normal.dotProduct(ray->dir);
        if ( c > 0.0 ) {
            double aFraction = fraction * (c * currentElement->area / projectedArea);
            double w = aFraction / currentElement->area / (double) globalNumberOfRays;
            currentElement->receivedRadiance[0].addScaled(currentElement->receivedRadiance[0], (float) w, rayPower);
        }
    } else {
        // Recursive case
        for ( int i = 0; currentElement != nullptr && currentElement->irregularSubElements != nullptr && i < currentElement->irregularSubElements->size(); i++ ) {
            stochasticJacobiPropagateRadianceClusterRecursive(
                (StochasticRadiosityElement *)currentElement->irregularSubElements->get(i),
                rayPower,
                ray,
                dir,
                projectedArea,
                fraction);
        }
    }
}

static void
stochasticJacobiPropagateRadianceToClusterOriented(
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
static void
stochasticJacobiReceiverProjectedAreaRecursive(
    const StochasticRadiosityElement *currentElement,
    Ray *ray,
    float dir,
    double *area)
{
    if ( currentElement != nullptr && !currentElement->isCluster() ) {
        // Trivial case
        double c = -dir * currentElement->patch->normal.dotProduct(ray->dir);
        if ( c > 0.0 ) {
            *area += c * currentElement->area;
        }
    } else {
        // Recursive case
        for ( int i = 0; currentElement != nullptr && currentElement->irregularSubElements != nullptr &&
                 i < currentElement->irregularSubElements->size(); i++ ) {
            stochasticJacobiReceiverProjectedAreaRecursive(
                (StochasticRadiosityElement *)currentElement->irregularSubElements->get(i),
                ray,
                dir,
                area);
        }
    }
}

static double
stochasticJacobiReceiverProjectedArea(const StochasticRadiosityElement *cluster, Ray *ray, float dir) {
    double area = 0.0;
    stochasticJacobiReceiverProjectedAreaRecursive(cluster, ray, dir, &area);
    return area;
}

/**
Transfer radiance from src to rcv.
src_prob = un-normalised src birth stochasticJacobiProbability / src area
rcv_prob = un-normalised rcv birth stochasticJacobiProbability / rcv area for bidirectional transfers
      or = 0 for unidirectional transfers
score will be weighted with globalSumProbabilities / nr_rays (both are global).
ray->dir and dir are used in order to determine projected cluster area
and cosine of incident direction of cluster surface elements when
the receiver is a cluster
*/
static void
stochasticJacobiPropagateRadiance(
    StochasticRadiosityElement *src,
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
    double weight = globalSumOfProbabilities / src_prob; // src area / normalised src prob
    double fraction = src_prob / (src_prob + rcv_prob); // 1 for uni-directional transfers

    if ( src_prob < EPSILON * EPSILON /* this should never happen */
         || fraction < EPSILON ) {
        // Reverse transfer from a black surface
        return;
    }

    radiance = stochasticJacobiGetSourceRadiance(src, us, vs);
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate ) {
        radiance.subtract(radiance, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance);
    }
    rayPower.scaledCopy((float) weight, radiance);

    if ( !rcv->isCluster() ) {
        stochasticJacobiPropagateRadianceToSurface(rcv, ur, vr, rayPower, src, fraction, weight);
    } else {
        switch ( GLOBAL_stochasticRaytracing_hierarchy.clustering ) {
            case NO_CLUSTERING:
                logFatal(-1, "Propagate", "hierarchyRefine() returns cluster although clustering is disabled.\n");
                break;
            case ISOTROPIC_CLUSTERING:
                stochasticJacobiPropagateRadianceToClusterIsotropic(rcv, rayPower, src, fraction, weight);
                break;
            case ORIENTED_CLUSTERING:
                area = stochasticJacobiReceiverProjectedArea(rcv, ray, dir);
                if ( area > EPSILON ) {
                    stochasticJacobiPropagateRadianceToClusterOriented(rcv, rayPower, ray, dir, src, area, fraction,
                                                                       weight);
                }
                break;
            default:
                logFatal(-1, "Propagate", "Invalid clustering mode %d\n", (int) GLOBAL_stochasticRaytracing_hierarchy.clustering);
        }
    }
}

/**
Idem but for importance
*/
static void
stochasticJacobiPropagateImportance(
    StochasticRadiosityElement *src,
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
    double w = globalSumOfProbabilities / (src_prob + rcv_prob) / rcv->area / (double) globalNumberOfRays;
    rcv->receivedImportance += (float)(w * stochasticRadiosityElementScalarReflectance(src) * globalGetImportanceCallback(src));

    if ( GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing || GLOBAL_stochasticRaytracing_hierarchy.clustering != NO_CLUSTERING ) {
        logFatal(-1, "Propagate", "Importance propagation not implemented in combination with hierarchical refinement");
    }
}

/**
Src is the leaf element containing the point from which to propagate
radiance on P. P and Q are toplevel surface elements. Transfer
is from P to Q
*/
static void
stochasticJacobiRefineAndPropagateRadiance(
    StochasticRadiosityElement *src,
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
    RenderOptions *renderOptions)
{
    LINK link{};
    link = topLink(Q, P);
    hierarchyRefine(&link, Q, &uq, &vq, P, &up, &vp, GLOBAL_stochasticRaytracing_hierarchy.oracle, renderOptions);
    // Propagate from the leaf element src to the admissible receiver element containing/contained by Q
    stochasticJacobiPropagateRadiance(src, us, vs, link.rcv, uq, vq, src_prob, rcv_prob, ray, dir);
}

static void
stochasticJacobiRefineAndPropagateImportance(
    StochasticRadiosityElement *P,
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
static void
stochasticJacobiRefineAndPropagate(
    StochasticRadiosityElement *P,
    double up,
    double vp,
    StochasticRadiosityElement *Q,
    double uq,
    double vq,
    Ray *ray,
    RenderOptions *renderOptions)
{
    double src_prob;
    double us = up;
    double vs = vp;
    StochasticRadiosityElement *src = stochasticRadiosityElementRegularLeafElementAtPoint(P, &us, &vs);
    src_prob = (double) src->samplingProbability / (double) src->area;
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.bidirectionalTransfers ) {
        double rcv_prob;
        double ur = uq;
        double vr = vq;
        StochasticRadiosityElement *rcv = stochasticRadiosityElementRegularLeafElementAtPoint(Q, &ur, &vr);
        rcv_prob = (double) rcv->samplingProbability / (double) rcv->area;

        if ( globalGetRadianceCallback ) {
            stochasticJacobiRefineAndPropagateRadiance(src, us, vs, P, up, vp, Q, uq, vq, src_prob, rcv_prob, ray, +1, renderOptions);
            stochasticJacobiRefineAndPropagateRadiance(rcv, ur, vr, Q, uq, vq, P, up, vp, rcv_prob, src_prob, ray, -1, renderOptions);
        }
        if ( globalGetImportanceCallback ) {
            stochasticJacobiRefineAndPropagateImportance(P, up, vp, Q, uq, vq, src_prob, rcv_prob, ray, +1);
            stochasticJacobiRefineAndPropagateImportance(Q, uq, vq, P, up, vp, rcv_prob, src_prob, ray, -1);
        }
    } else {
        if ( globalGetRadianceCallback ) {
            stochasticJacobiRefineAndPropagateRadiance(src, us, vs, P, up, vp, Q, uq, vq, src_prob, 0.0, ray, +1, renderOptions);
        }
        if ( globalGetImportanceCallback ) {
            stochasticJacobiRefineAndPropagateImportance(P, up, vp, Q, uq, vq, src_prob, 0.0, ray, +1);
        }
    }
}

static double *
stochasticJacobiNextSample(
    StochasticRadiosityElement *elem,
    int nMostSignificantBit,
    niedindex mostSignificantBit1,
    niedindex rMostSignificantBit2,
    double *zeta)
{
    niedindex *xi;
    niedindex u;
    niedindex v;
    // Use different ray index for propagating importance and radiance
    niedindex *ray_index = globalGetRadianceCallback ? &elem->rayIndex : &elem->importanceRayIndex;

    xi = NextNiedInRange(ray_index, +1, nMostSignificantBit, mostSignificantBit1, rMostSignificantBit2);

    (*ray_index)++;
    u = (xi[0] & ~3) | 1; // Avoid positions on sub-element boundaries
    v = (xi[1] & ~3) | 1;
    if ( elem->numberOfVertices == 3 ) {
        foldSample(&u, &v);
    }
    zeta[0] = (double) u * RECIP;
    zeta[1] = (double) v * RECIP;
    zeta[2] = (double) xi[2] * RECIP;
    zeta[3] = (double) xi[3] * RECIP;
    return zeta;
}

/**
Determines uniform (u,v) parameters of hit point on hit patch
*/
static void
stochasticJacobiUniformHitCoordinates(const RayHit *hit, double *uHit, double *vHit) {
    if ( hit->getFlags() & HIT_UV ) {
        // (u,v) coordinates obtained as side result of intersection test
        *uHit = hit->getUv().u;
        *vHit = hit->getUv().v;
        if ( hit->getPatch()->jacobian ) {
            hit->getPatch()->biLinearToUniform(uHit, vHit);
        }
    } else {
        Vector3D position = hit->getPoint();
        hit->getPatch()->uniformUv(&position, uHit, vHit);
    }

    // Clip uv coordinates to lay strictly inside the hit patch
    if ( *uHit < EPSILON ) {
        *uHit = EPSILON;
    }
    if ( *vHit < EPSILON ) {
        *vHit = EPSILON;
    }
    if ( *uHit > 1.0 - EPSILON ) {
        *uHit = 1.0 - EPSILON;
    }
    if ( *vHit > 1.0 - EPSILON ) {
        *vHit = 1.0 - EPSILON;
    }
}

/**
Traces a local line from 'src' and propagates radiance and/or importance from P to
hit patch (and back for bidirectional transfers)
*/
static void
stochasticJacobiElementShootRay(
    VoxelGrid * sceneWorldVoxelGrid,
    StochasticRadiosityElement *src,
    int nMostSignificantBit,
    niedindex mostSignificantBit1,
    niedindex rMostSignificantBit2,
    RenderOptions *renderOptions)
{
    if ( globalGetRadianceCallback != nullptr ) {
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays++;
    }

    if ( globalGetImportanceCallback != nullptr ) {
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays++;
    }

    double zeta[4];
    Ray ray = mcrGenerateLocalLine(src->patch,
                               stochasticJacobiNextSample(src, nMostSignificantBit, mostSignificantBit1, rMostSignificantBit2, zeta));

    RayHit hitStore;
    const RayHit *hit = mcrShootRay(sceneWorldVoxelGrid, src->patch, &ray, &hitStore);

    if ( hit ) {
        double uHit = 0.0;
        double vHit = 0.0;
        stochasticJacobiUniformHitCoordinates(hit, &uHit, &vHit);
        stochasticJacobiRefineAndPropagate(topLevelStochasticRadiosityElement(src->patch), zeta[0], zeta[1],
                                           topLevelStochasticRadiosityElement(hit->getPatch()), uHit, vHit, &ray, renderOptions);
    } else {
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.numberOfMisses++;
    }
}

static void
stochasticJacobiInitPushRayIndex(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = (StochasticRadiosityElement *)element;

    if ( stochasticRadiosityElement == nullptr ) {
        return;
    }
    stochasticRadiosityElement->rayIndex = ((StochasticRadiosityElement *)stochasticRadiosityElement->parent)->rayIndex;
    stochasticRadiosityElement->importanceRayIndex = ((StochasticRadiosityElement *)stochasticRadiosityElement->parent)->importanceRayIndex;
    stochasticRadiosityElement->traverseAllChildren(stochasticJacobiInitPushRayIndex);
}

/**
Determines nr of rays to shoot from element and shoots this number of rays
*/
static void
stochasticJacobiElementShootRays(
    VoxelGrid *sceneWorldVoxelGrid,
    StochasticRadiosityElement *element,
    int raysThisElem,
    RenderOptions *renderOptions)
{
    int sampleRange; // Determines a range in which to generate a sample
    niedindex mostSignificantBit1; // See monteCarloRadiosityElementRange() and NextSample()
    niedindex rMostSignificantBit2;

    // Sample number range for 4D Niederreiter sequence
    stochasticRadiosityElementRange(element, &sampleRange, &mostSignificantBit1, &rMostSignificantBit2);

    // Shoot the rays
    for ( int i = 0; i < raysThisElem; i++ ) {
        stochasticJacobiElementShootRay(sceneWorldVoxelGrid, element, sampleRange, mostSignificantBit1, rMostSignificantBit2, renderOptions);
    }

    if ( element != nullptr && !element->isLeaf() ) {
        // Source got subdivided while shooting the rays
        element->traverseAllChildren(stochasticJacobiInitPushRayIndex);
    }
}

static void
stochasticJacobiShootRaysRecursive(
    VoxelGrid *sceneWorldVoxelGrid,
    StochasticRadiosityElement *element,
    double rnd,
    long *rayCount,
    double *cumulative,
    RenderOptions *renderOptions) {
    if ( element->regularSubElements == nullptr ) {
        // Trivial case
        double p = element->samplingProbability / globalSumOfProbabilities;
        long rays_this_leaf =
                (long) std::floor((*cumulative + p) * (double) globalNumberOfRays + rnd) - *rayCount;

        if ( rays_this_leaf > 0 ) {
            stochasticJacobiElementShootRays(sceneWorldVoxelGrid, element, (int)rays_this_leaf, renderOptions);
        }

        *cumulative += p;
        *rayCount += rays_this_leaf;
    } else {
        // Recursive case
        for ( int i = 0; i < 4; i++ ) {
            stochasticJacobiShootRaysRecursive(
                sceneWorldVoxelGrid,
                (StochasticRadiosityElement *)element->regularSubElements[i],
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
static void
stochasticJacobiShootRays(
    VoxelGrid *sceneWorldVoxelGrid,
    const java::ArrayList<Patch *> *scenePatches,
    RenderOptions *renderOptions)
{
    double rnd = drand48();
    long rayCount = 0;
    double cumulative = 0.0;

    // Loop over all leaf elements in the element hierarchy
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        stochasticJacobiShootRaysRecursive(
            sceneWorldVoxelGrid,
            topLevelStochasticRadiosityElement(scenePatches->get(i)),
            rnd,
            &rayCount,
            &cumulative,
            renderOptions);
    }

    fprintf(stderr, "\n");
}

/**
Converts received radiance and importance at a leaf element into a new
approximation of total and un-shot radiance and importance
*/
static void
stochasticJacobiUpdateElement(StochasticRadiosityElement *elem) {
    if ( globalGetRadianceCallback ) {
        if ( globalDoControlVariate ) {
            // Add constant radiosity contribution to received flux
            elem->receivedRadiance[0].add(
                elem->receivedRadiance[0], GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance);
        }
        // Multiply with reflectivity on leaf elements only
        stochasticRadiosityMultiplyCoefficients(elem->Rd, elem->receivedRadiance, elem->basis);
    }

    globalReflectCallback(elem, (double) globalNumberOfRays / globalSumOfProbabilities);

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.addScaled(
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux,
        (float)M_PI * elem->area,
        elem->unShotRadiance[0]);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.addScaled(
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux,
        (float)M_PI * elem->area,
        elem->radiance[0]);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux.addScaled(
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux,
        (float)M_PI * elem->area * (elem->importance - elem->sourceImportance),
        elem->unShotRadiance[0]);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp += (elem->area * std::fabs(elem->unShotImportance));
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp += elem->area * elem->importance;
}

static void
stochasticJacobiPush(StochasticRadiosityElement *parent, StochasticRadiosityElement *child) {
    if ( globalGetRadianceCallback ) {
        ColorRgb Rd;
        Rd.clear();

        if ( parent->isCluster() && !child->isCluster() ) {
            // Multiply with reflectance (See PropagateRadianceToClusterIsotropic() above)
            ColorRgb rad = parent->receivedRadiance[0];
            Rd = child->Rd;
            rad.selfScalarProduct(Rd);
            stochasticRadiosityElementPushRadiance(parent, child, &rad, child->receivedRadiance);
        } else
            stochasticRadiosityElementPushRadiance(parent, child, parent->receivedRadiance, child->receivedRadiance);
    }

    if ( globalGetImportanceCallback ) {
        stochasticRadiosityElementPushImportance(&parent->receivedImportance, &child->receivedImportance);
    }
}

static void
stochasticJacobiPull(StochasticRadiosityElement *parent, StochasticRadiosityElement *child) {
    if ( globalGetRadianceCallback ) {
        stochasticRadiosityElementPullRadiance(parent, child, parent->radiance, child->radiance);
        stochasticRadiosityElementPullRadiance(parent, child, parent->unShotRadiance, child->unShotRadiance);
    }
    if ( globalGetImportanceCallback ) {
        stochasticRadiosityElementPullImportance(parent, child, &parent->importance, &child->importance);
        stochasticRadiosityElementPullImportance(parent, child, &parent->unShotImportance, &child->unShotImportance);
    }
}

/**
Clears everything to be pulled from children elements to zero
*/
static void
stochasticJacobiClearElement(StochasticRadiosityElement *parent) {
    if ( globalGetRadianceCallback ) {
        stochasticRadiosityClearCoefficients(parent->radiance, parent->basis);
        stochasticRadiosityClearCoefficients(parent->unShotRadiance, parent->basis);
    }
    if ( globalGetImportanceCallback ) {
        parent->importance = parent->unShotImportance = 0.0;
    }
}

static void stochasticJacobiPushUpdatePull(Element *elem);

static void
stochasticJacobiPushUpdatePullChild(Element *element) {
    StochasticRadiosityElement *child = (StochasticRadiosityElement *)element;
    StochasticRadiosityElement *parent = (StochasticRadiosityElement *)child->parent;
    stochasticJacobiPush(parent, child);
    stochasticJacobiPushUpdatePull(child);
    stochasticJacobiPull(parent, child);
}

static void
stochasticJacobiPushUpdatePull(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = (StochasticRadiosityElement *)element;
    if ( stochasticRadiosityElement != nullptr && stochasticRadiosityElement->isLeaf() ) {
        stochasticJacobiUpdateElement(stochasticRadiosityElement);
    } else if ( element != nullptr ) {
        // Not a leaf element
        stochasticJacobiClearElement(stochasticRadiosityElement);
        element->traverseAllChildren(stochasticJacobiPushUpdatePullChild);
    }
}

static void
stochasticJacobiPullRdEd(StochasticRadiosityElement *element);

static void
stochasticJacobiPullRdEdFromChild(Element *element) {
    StochasticRadiosityElement *child = (StochasticRadiosityElement *)element;
    StochasticRadiosityElement *parent = (StochasticRadiosityElement *)child->parent;

    stochasticJacobiPullRdEd(child);

    parent->Ed.addScaled(parent->Ed, child->area / parent->area, child->Ed);
    parent->Rd.addScaled(parent->Rd, child->area / parent->area, child->Rd);
    if ( parent->isCluster() )
        parent->Rd.setMonochrome(1.0);
}

static void
stochasticJacobiPullRdEd(StochasticRadiosityElement *element) {
    if ( element == nullptr || element->isLeaf() || (!element->isCluster() && !stochasticRadiosityElementIsTextured(element)) ) {
        return;
    }

    element->Ed.clear();
    element->Rd.clear();
    element->traverseAllChildren(stochasticJacobiPullRdEdFromChild);
}

static void
stochasticJacobiPushUpdatePullSweep() {
    // Update radiance, compute new total and un-shot flux
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.clear();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp = 0.0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.clear();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp = 0.0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux.clear();

    // Update reflectances and emittances (refinement yields more accurate estimates
    // on textured surfaces)
    stochasticJacobiPullRdEd(GLOBAL_stochasticRaytracing_hierarchy.topCluster);

    stochasticJacobiPushUpdatePull(GLOBAL_stochasticRaytracing_hierarchy.topCluster);
}

/**
Generic routine for Stochastic Jacobi iterations:
- nr_rays: nr of rays to use
- GetRadiance: routine returning radiance (total or un-shot) to be
propagated for a given element, or nullptr if no radiance propagation is
required.
- GetImportance: same, but for importance.
- Update: routine updating total, un-shot and source radiance and/or
importance based on result received during the iteration.

The operation of this routine is further controlled by global parameters
- GLOBAL_stochasticRaytracing_monteCarloRadiosityState.do_control_radiosity: perform constant control variate variance reduction
- GLOBAL_stochasticRaytracing_monteCarloRadiosityState.bidirectionalTransfers: for using lines bidirectionally
- GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven: importance-driven radiance propagation
- GLOBAL_stochasticRaytracing_monteCarloRadiosityState.radianceDriven: radiance-driven importance propagation
- hierarchy.do_h_meshing, hierarchy.clustering: hierarchical refinement/clustering

This routine updates global ray counts and total/un-shot power/importance statistics.

CAVEAT: propagate either radiance or importance alone. Simultaneous
propagation of importance and radiance does not work yet.
*/
void
doStochasticJacobiIteration(
    VoxelGrid *sceneWorldVoxelGrid,
    long numberOfRays,
    ColorRgb *(*GetRadiance)(StochasticRadiosityElement *),
    float (*GetImportance)(StochasticRadiosityElement *),
    void (*Update)(StochasticRadiosityElement *P, double w),
    java::ArrayList<Patch *> *scenePatches,
    RenderOptions *renderOptions)
{
    stochasticJacobiInitGlobals((int)numberOfRays, GetRadiance, GetImportance, Update);
    stochasticJacobiPrintMessage(numberOfRays);
    if ( !stochasticJacobiSetup(scenePatches) ) {
        return;
    }
    stochasticJacobiShootRays(sceneWorldVoxelGrid, scenePatches, renderOptions);
    stochasticJacobiPushUpdatePullSweep();
}

#endif
