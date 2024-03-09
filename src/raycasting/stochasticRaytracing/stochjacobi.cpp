/**
Generic stochastic Jacobi iteration (local lines)
TODO: combined radiance/importance propagation
TODO: hierarchical refinement for importance propagation
TODO: re-incorporate the rejection sampling technique for
sampling positions on shooters with higher order radiosity approximation
(lower variance)
TODO: global lines and global line bundles.
*/

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "raycasting/stochasticRaytracing/ccr.h"
#include "raycasting/stochasticRaytracing/localline.h"

// Returns radiance or importance to be propagated
static COLOR *(*globalGetRadianceCallback)(StochasticRadiosityElement *);

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
    COLOR *(*GetRadiance)(StochasticRadiosityElement *),
    float (*GetImportance)(StochasticRadiosityElement *),
    void (*Update)(StochasticRadiosityElement *P, double w))
{
    globalNumberOfRays = numberOfRays;
    globalGetRadianceCallback = GetRadiance;
    globalGetImportanceCallback = GetImportance;
    globalReflectCallback = Update;
    // Only use control variates for propagating radiance, not for importance
    globalDoControlVariate = (GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate && (GetRadiance));

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
        fprintf(stderr, "%sradiance ",
                GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ? "importance-driven " : "");
    }
    if ( globalGetRadianceCallback && globalGetImportanceCallback ) {
        fprintf(stderr, "and ");
    }
    if ( globalGetImportanceCallback ) {
        fprintf(stderr, "%simportance ",
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
        COLOR radiance = globalGetRadianceCallback(elem)[0];
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate ) {
            colorSubtract(radiance, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance, radiance);
        }
        prob = /* M_PI * */ elem->area * colorSumAbsComponents(radiance);
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
            // Weight with received importance
            float w = elem->imp - elem->source_imp;
            prob *= ((w > 0.0) ? w : 0.0);
        }
    }

    if ( globalGetImportanceCallback && GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
        double prob2 = elem->area * fabs(globalGetImportanceCallback(elem)) * monteCarloRadiosityElementScalarReflectance(elem);

        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.radianceDriven ) {
            // Received-radiance weighted importance transport
            COLOR received_radiance;
            colorSubtract(elem->radiance[0], elem->sourceRad, received_radiance);
            prob2 *= colorSumAbsComponents(received_radiance);
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
        elem->received_imp = 0.0;
    }
}

/**
Clears received radiance and importance and accumulates the un-normalized
sampling probabilities at leaf elements
*/
static void
stochasticJacobiElementSetup(StochasticRadiosityElement *elem) {
    elem->prob = 0.0;
    if ( !monteCarloRadiosityForAllChildrenElements(elem, stochasticJacobiElementSetup)) {
        // Elem is a leaf element. We need to compute the sum of the un-normalized
        // sampling "probabilities" at the leaf elements
        elem->prob = (float)stochasticJacobiProbability(elem);
        globalSumOfProbabilities += elem->prob;
    }
    if ( elem->parent ) {
        // The probability of sampling a non-leaf element is the sum of the
        // probabilities of sampling the sub-elements
        ((StochasticRadiosityElement *)elem->parent)->prob += elem->prob;
    }

    stochasticJacobiElementClearAccumulators(elem);
}

/**
Returns true if success, that is: sum of sampling probabilities is nonzero
*/
static int
stochasticJacobiSetup(java::ArrayList<Patch *> *scenePatches) {
    // Determine constant control radiosity if required
    colorClear(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance);
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
static COLOR
stochasticJacobiGetSourceRadiance(StochasticRadiosityElement *src, double us, double vs) {
    COLOR *srcRad = globalGetRadianceCallback(src);
    return colorAtUv(src->basis, srcRad, us, vs);
}

static void
stochasticJacobiPropagateRadianceToSurface(
    StochasticRadiosityElement *rcv,
    double ur,
    double vr,
    COLOR rayPower,
    StochasticRadiosityElement * /*src*/,
    double fraction,
    double /*weight*/)
{
    for ( int i = 0; i < rcv->basis->size; i++ ) {
        double dual = rcv->basis->dualFunction[i](ur, vr) / rcv->area;
        double w = dual * fraction / (double) globalNumberOfRays;
        colorAddScaled(rcv->receivedRadiance[i], (float)w, rayPower, rcv->receivedRadiance[i]);
    }
}

static void
stochasticJacobiPropagateRadianceToClusterIsotropic(
    StochasticRadiosityElement *cluster,
    COLOR rayPower,
    StochasticRadiosityElement * /*src*/,
    double fraction,
    double /*weight*/)
{
    double w = fraction / cluster->area / (double) globalNumberOfRays;
    colorAddScaled(cluster->receivedRadiance[0], (float)w, rayPower, cluster->receivedRadiance[0]);
}

/**
Note: Not considering the MAX_HIERARCHY_DEPTH limit.
*/
static void
stochasticJacobiPropagateRadianceClusterRecursive(
    StochasticRadiosityElement *currentElement,
    COLOR rayPower,
    Ray *ray,
    float dir,
    double projectedArea,
    double fraction)
{
    if ( currentElement != nullptr && !currentElement->isCluster ) {
        // Trivial case
        double c = -dir * vectorDotProduct(currentElement->patch->normal, ray->dir);
        if ( c > 0. ) {
            double aFraction = fraction * (c * currentElement->area / projectedArea);
            double w = aFraction / currentElement->area / (double) globalNumberOfRays;
            colorAddScaled(currentElement->receivedRadiance[0], (float)w, rayPower, currentElement->receivedRadiance[0]);
        }
    } else {
        // Recursive case
        for ( int i = 0; currentElement->irregularSubElements != nullptr && i < currentElement->irregularSubElements->size(); i++ ) {
            stochasticJacobiPropagateRadianceClusterRecursive(
                currentElement->irregularSubElements->get(i),
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
    COLOR rayPower,
    Ray *ray,
    float dir,
    StochasticRadiosityElement * /*src*/,
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
    StochasticRadiosityElement *currentElement,
    Ray *ray,
    float dir,
    double *area)
{
    if ( currentElement != nullptr && !currentElement->isCluster ) {
        // Trivial case
        double c = -dir * vectorDotProduct(currentElement->patch->normal, ray->dir);
        if ( c > 0.0 ) {
            *area += c * currentElement->area;
        }
    } else {
        // Recursive case
        for ( int i = 0; currentElement->irregularSubElements != nullptr &&
                 i < currentElement->irregularSubElements->size(); i++ ) {
            stochasticJacobiReceiverProjectedAreaRecursive(
                currentElement->irregularSubElements->get(i),
                ray,
                dir,
                area);
        }
    }
}

static double
stochasticJacobiReceiverProjectedArea(StochasticRadiosityElement *cluster, Ray *ray, float dir) {
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
    COLOR rad;
    COLOR rayPower;
    double area;
    double weight = globalSumOfProbabilities / src_prob; // src area / normalised src prob
    double fraction = src_prob / (src_prob + rcv_prob); // 1 for uni-directional transfers

    if ( src_prob < EPSILON * EPSILON /* this should never happen */
         || fraction < EPSILON ) {
        // Reverse transfer from a black surface
        return;
    }

    rad = stochasticJacobiGetSourceRadiance(src, us, vs);
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate ) {
        colorSubtract(rad, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance, rad);
    }
    colorScale((float)weight, rad, rayPower);

    if ( !rcv->isCluster ) {
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
    Ray * /*ray*/,
    float /*dir*/)
{
    double w = globalSumOfProbabilities / (src_prob + rcv_prob) / rcv->area / (double) globalNumberOfRays;
    rcv->received_imp += (float)(w * monteCarloRadiosityElementScalarReflectance(src) * globalGetImportanceCallback(src));

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
    float dir)
{
    LINK link{};
    link = topLink(Q, P);
    hierarchyRefine(&link, Q, &uq, &vq, P, &up, &vp, GLOBAL_stochasticRaytracing_hierarchy.oracle);
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
    Ray *ray,
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
    Ray *ray)
{
    double src_prob;
    double us = up;
    double vs = vp;
    StochasticRadiosityElement *src = monteCarloRadiosityRegularLeafElementAtPoint(P, &us, &vs);
    src_prob = (double) src->prob / (double) src->area;
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.bidirectionalTransfers ) {
        double rcv_prob;
        double ur = uq, vr = vq;
        StochasticRadiosityElement *rcv = monteCarloRadiosityRegularLeafElementAtPoint(Q, &ur, &vr);
        rcv_prob = (double) rcv->prob / (double) rcv->area;

        if ( globalGetRadianceCallback ) {
            stochasticJacobiRefineAndPropagateRadiance(src, us, vs, P, up, vp, Q, uq, vq, src_prob, rcv_prob, ray, +1);
            stochasticJacobiRefineAndPropagateRadiance(rcv, ur, vr, Q, uq, vq, P, up, vp, rcv_prob, src_prob, ray, -1);
        }
        if ( globalGetImportanceCallback ) {
            stochasticJacobiRefineAndPropagateImportance(P, up, vp, Q, uq, vq, src_prob, rcv_prob, ray, +1);
            stochasticJacobiRefineAndPropagateImportance(Q, uq, vq, P, up, vp, rcv_prob, src_prob, ray, -1);
        }
    } else {
        if ( globalGetRadianceCallback ) {
            stochasticJacobiRefineAndPropagateRadiance(src, us, vs, P, up, vp, Q, uq, vq, src_prob, 0., ray, +1);
        }
        if ( globalGetImportanceCallback ) {
            stochasticJacobiRefineAndPropagateImportance(P, up, vp, Q, uq, vq, src_prob, 0., ray, +1);
        }
    }
}

static double *
stochasticJacobiNextSample(
    StochasticRadiosityElement *elem,
    int nmsb,
    niedindex msb1,
    niedindex rmsb2,
    double *zeta)
{
    niedindex *xi;
    niedindex u;
    niedindex v;
    // Use different ray index for propagating importance and radiance
    niedindex *ray_index = globalGetRadianceCallback ? &elem->ray_index : &elem->imp_ray_index;

    xi = NextNiedInRange(ray_index, +1, nmsb, msb1, rmsb2);

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
stochasticJacobiUniformHitCoordinates(RayHit *hit, double *uHit, double *vHit) {
    if ( hit->flags & HIT_UV ) {
        // (u,v) coordinates obtained as side result of intersection test
        *uHit = hit->uv.u;
        *vHit = hit->uv.v;
        if ( hit->patch->jacobian ) {
            hit->patch->biLinearToUniform(uHit, vHit);
        }
    } else {
        hit->patch->uniformUv(&hit->point, uHit, vHit);
    }

    // Clip uv coordinates to lay strictly inside the hit patch
    if ( *uHit < EPSILON ) {
        *uHit = EPSILON;
    }
    if ( *vHit < EPSILON ) {
        *vHit = EPSILON;
    }
    if ( *uHit > 1. - EPSILON ) {
        *uHit = 1. - EPSILON;
    }
    if ( *vHit > 1. - EPSILON ) {
        *vHit = 1. - EPSILON;
    }
}

/**
Traces a local line from 'src' and propagates radiance and/or importance from P to
hit patch (and back for bidirectional transfers)
*/
static void
stochasticJacobiElementShootRay(
    StochasticRadiosityElement *src,
    int nmsb,
    niedindex msb1,
    niedindex rmsb2)
{
    if ( globalGetRadianceCallback != nullptr ) {
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays++;
    }

    if ( globalGetImportanceCallback != nullptr ) {
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays++;
    }

    double zeta[4];
    Ray ray = mcrGenerateLocalLine(src->patch,
                               stochasticJacobiNextSample(src, nmsb, msb1, rmsb2, zeta));

    RayHit hitStore;
    RayHit *hit = mcrShootRay(src->patch, &ray, &hitStore);

    if ( hit ) {
        double uHit = 0.0;
        double vHit = 0.0;
        stochasticJacobiUniformHitCoordinates(hit, &uHit, &vHit);
        stochasticJacobiRefineAndPropagate(topLevelGalerkinElement(src->patch), zeta[0], zeta[1],
                                           topLevelGalerkinElement(hit->patch), uHit, vHit, &ray);
    } else {
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.numberOfMisses++;
    }
}

static void
stochasticJacobiInitPushRayIndex(StochasticRadiosityElement *elem) {
    elem->ray_index = ((StochasticRadiosityElement *)elem->parent)->ray_index;
    elem->imp_ray_index = ((StochasticRadiosityElement *)elem->parent)->imp_ray_index;
    monteCarloRadiosityForAllChildrenElements(elem, stochasticJacobiInitPushRayIndex);
}

/**
Determines nr of rays to shoot from elem and shoots this number of rays
*/
static void
stochasticJacobiElementShootRays(StochasticRadiosityElement *elem, int rays_this_elem) {
    int i;
    int sampleRange; // Determines a range in which to generate a sample
    niedindex msb1; // See monteCarloRadiosityElementRange() and NextSample()
    niedindex rmsb2;

    // Sample number range for 4D Niederreiter sequence
    monteCarloRadiosityElementRange(elem, &sampleRange, &msb1, &rmsb2);

    // Shoot the rays
    for ( i = 0; i < rays_this_elem; i++ ) {
        stochasticJacobiElementShootRay(elem, sampleRange, msb1, rmsb2);
    }

    if ( !monteCarloRadiosityElementIsLeaf(elem)) {
        // Source got subdivided while shooting the rays
        monteCarloRadiosityForAllChildrenElements(elem, stochasticJacobiInitPushRayIndex);
    }
}

static void
stochasticJacobiShootRaysRecursive(StochasticRadiosityElement *element, double rnd, long *rayCount, double *pCumulative) {
    if ( element->regularSubElements == nullptr ) {
        // Trivial case
        double p = element->prob / globalSumOfProbabilities;
        long rays_this_leaf =
                (long) std::floor((*pCumulative + p) * (double) globalNumberOfRays + rnd) - *rayCount;

        if ( rays_this_leaf > 0 ) {
            stochasticJacobiElementShootRays(element, (int)rays_this_leaf);
        }

        *pCumulative += p;
        *rayCount += rays_this_leaf;
    } else {
        // Recursive case
        for ( int i = 0; i < 4; i++ ) {
            stochasticJacobiShootRaysRecursive((StochasticRadiosityElement *)element->regularSubElements[i], rnd, rayCount, pCumulative);
        }
    }
}

/**
Fire off rays from the leaf elements, propagate radiance/importance
*/
static void
stochasticJacobiShootRays(java::ArrayList<Patch *> *scenePatches) {
    double rnd = drand48();
    long rayCount = 0;
    double pCumulative = 0.0;

    // Loop over all leaf elements in the element hierarchy
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        stochasticJacobiShootRaysRecursive(topLevelGalerkinElement(scenePatches->get(i)), rnd, &rayCount, &pCumulative);
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
            colorAdd(elem->receivedRadiance[0], GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance, elem->receivedRadiance[0]);
        }
        // Multiply with reflectivity on leaf elements only
        stochasticRadiosityMultiplyCoefficients(elem->Rd, elem->receivedRadiance, elem->basis);
    }

    globalReflectCallback(elem, (double) globalNumberOfRays / globalSumOfProbabilities);

    colorAddScaled(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux, M_PI * elem->area, elem->unShotRadiance[0], GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux);
    colorAddScaled(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux, M_PI * elem->area, elem->radiance[0], GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux);
    colorAddScaled(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux, M_PI * elem->area * (elem->imp - elem->source_imp), elem->unShotRadiance[0],
                   GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp += elem->area * fabs(elem->unShotImp);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp += elem->area * elem->imp;
}

static void
stochasticJacobiPush(StochasticRadiosityElement *parent, StochasticRadiosityElement *child) {
    if ( globalGetRadianceCallback ) {
        COLOR Rd;
        colorClear(Rd);

        if ( parent->isCluster && !child->isCluster ) {
            // Multiply with reflectance (See PropagateRadianceToClusterIsotropic() above)
            COLOR rad = parent->receivedRadiance[0];
            Rd = child->Rd;
            colorProduct(Rd, rad, rad);
            pushRadiance(parent, child, &rad, child->receivedRadiance);
        } else
            pushRadiance(parent, child, parent->receivedRadiance, child->receivedRadiance);
    }

    if ( globalGetImportanceCallback ) {
        pushImportance(parent, child, &parent->received_imp, &child->received_imp);
    }
}

static void
stochasticJacobiPull(StochasticRadiosityElement *parent, StochasticRadiosityElement *child) {
    if ( globalGetRadianceCallback ) {
        pullRadiance(parent, child, parent->radiance, child->radiance);
        pullRadiance(parent, child, parent->unShotRadiance, child->unShotRadiance);
    }
    if ( globalGetImportanceCallback ) {
        pullImportance(parent, child, &parent->imp, &child->imp);
        pullImportance(parent, child, &parent->unShotImp, &child->unShotImp);
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
        parent->imp = parent->unShotImp = 0.;
    }
}

static void stochasticJacobiPushUpdatePull(StochasticRadiosityElement *elem);

static void
stochasticJacobiPushUpdatePullChild(StochasticRadiosityElement *child) {
    StochasticRadiosityElement *parent = (StochasticRadiosityElement *)child->parent;
    stochasticJacobiPush(parent, child);
    stochasticJacobiPushUpdatePull(child);
    stochasticJacobiPull(parent, child);
}

static void
stochasticJacobiPushUpdatePull(StochasticRadiosityElement *elem) {
    if ( monteCarloRadiosityElementIsLeaf(elem)) {
        stochasticJacobiUpdateElement(elem);
    } else {
        // Not a leaf element
        stochasticJacobiClearElement(elem);
        monteCarloRadiosityForAllChildrenElements(elem, stochasticJacobiPushUpdatePullChild);
    }
}

static void
stochasticJacobiPullRdEd(StochasticRadiosityElement *elem);

static void
stochasticJacobiPullRdEdFromChild(StochasticRadiosityElement *child) {
    StochasticRadiosityElement *parent = (StochasticRadiosityElement *)child->parent;

    stochasticJacobiPullRdEd(child);

    colorAddScaled(parent->Ed, child->area / parent->area, child->Ed, parent->Ed);
    colorAddScaled(parent->Rd, child->area / parent->area, child->Rd, parent->Rd);
    if ( parent->isCluster )
        colorSetMonochrome(parent->Rd, 1.);
}

static void
stochasticJacobiPullRdEd(StochasticRadiosityElement *elem) {
    if ( monteCarloRadiosityElementIsLeaf(elem) || (!elem->isCluster && !monteCarloRadiosityElementIsTextured(elem))) {
        return;
    }

    colorClear(elem->Ed);
    colorClear(elem->Rd);
    monteCarloRadiosityForAllChildrenElements(elem, stochasticJacobiPullRdEdFromChild);
}

static void
stochasticJacobiPushUpdatePullSweep() {
    // Update radiance, compute new total and un-shot flux
    colorClear(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp = 0.;
    colorClear(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp = 0.;
    colorClear(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux);

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
    long nr_rays,
    COLOR *(*GetRadiance)(StochasticRadiosityElement *),
    float (*GetImportance)(StochasticRadiosityElement *),
    void (*Update)(StochasticRadiosityElement *P, double w),
    java::ArrayList<Patch *> *scenePatches)
{
    stochasticJacobiInitGlobals((int)nr_rays, GetRadiance, GetImportance, Update);
    stochasticJacobiPrintMessage(nr_rays);
    if ( !stochasticJacobiSetup(scenePatches) ) {
        return;
    }
    stochasticJacobiShootRays(scenePatches);
    stochasticJacobiPushUpdatePullSweep();
}
