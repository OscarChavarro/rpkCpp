/**
Hierarchical element refinement.

References:
Hanrahan, Pat. et al, "A rapid hierarchical radiosity algorithm", SIGGRAPH 1991
Smits, Brian. et al, "An importance-driven radiosity algorithm", SIGGRAPH 1992
*/

#include "material/statistics.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"

/**
Refinement action 1: do nothing (link is accurate enough)
Special refinement action used to indicate that a link is admissible
*/
static LINK *
dontRefine(
        LINK *link,
        StochasticRadiosityElement *rcvtop,
        double *ur,
        double *vr,
        StochasticRadiosityElement *srctop,
        double *us,
        double *vs)
{
    // Doesn't do anything
    return link;
}

/**
Refinement action 2: subdivide the receiver using regular quadtree subdivision
*/
static LINK *
subdivideReceiver(
        LINK *link,
        StochasticRadiosityElement *rcvtop,
        double *ur,
        double *vr,
        StochasticRadiosityElement *srctop,
        double *us,
        double *vs)
{
    StochasticRadiosityElement *rcv = link->rcv;
    if ( rcv->isCluster ) {
        rcv = monteCarloRadiosityClusterChildContainingElement(rcv, rcvtop);
    } else {
        if ( !rcv->regularSubElements ) {
            monteCarloRadiosityRegularSubdivideElement(rcv);
        }
        rcv = monteCarloRadiosityRegularSubElementAtPoint(rcv, ur, vr);
    }
    link->rcv = rcv;
    return link;
}

/**
Refinement action 3: subdivide the source using regular quadtree subdivision
*/
static LINK *
subdivideSource(
        LINK *link,
        StochasticRadiosityElement *rcvtop,
        double *ur,
        double *vr,
        StochasticRadiosityElement *srctop,
        double *us,
        double *vs)
{
    StochasticRadiosityElement *src = link->src;
    if ( src->isCluster ) {
        src = monteCarloRadiosityClusterChildContainingElement(src, srctop);
    } else {
        if ( !src->regularSubElements ) {
            monteCarloRadiosityRegularSubdivideElement(src);
        }
        src = monteCarloRadiosityRegularSubElementAtPoint(src, us, vs);
    }
    link->src = src;
    return link;
}

static int
selfLink(LINK *link) {
    return (link->rcv == link->src);
}

static int
linkInvolvingClusters(LINK *link) {
    return (link->rcv->isCluster || link->src->isCluster);
}

static int
disjunctElements(StochasticRadiosityElement *rcv, StochasticRadiosityElement *src) {
    BoundingBox rcvbounds, srcbounds;
    monteCarloRadiosityElementBounds(rcv, rcvbounds);
    monteCarloRadiosityElementBounds(src, srcbounds);
    return disjunctBounds(rcvbounds, srcbounds);
}

/* Cheap form factor estimate to drive hierarchical refinement 
 * as in Hanrahan SIGGRAPH'91. */
static float formfactor_estimate(StochasticRadiosityElement *rcv, StochasticRadiosityElement *src) {
    Vector3D D;
    double d, c1, c2, f, f2;
    vectorSubtract(src->midpoint, rcv->midpoint, D);
    d = vectorNorm(D);
    f = src->area / (M_PI * d * d + src->area);
    f2 = 2. * f;
    c1 = rcv->isCluster ? 1. /*0.25*/ : fabs(vectorDotProduct(D, rcv->patch->normal)) / d;
    if ( c1 < f2 ) {
        c1 = f2;
    }
    c2 = src->isCluster ? 1. /*0.25*/ : fabs(vectorDotProduct(D, src->patch->normal)) / d;
    if ( c2 < f2 ) {
        c2 = f2;
    }
    return f * c1 * c2;
}

static int LowPowerLink(LINK *link) {
    StochasticRadiosityElement *rcv = link->rcv;
    StochasticRadiosityElement *src = link->src;
    COLOR rhosrcrad;
    float ff = formfactor_estimate(rcv, src);
    float threshold, propagated_power;

    /* compute receiver reflectance times source radiosity */
    colorScale(M_PI, src->rad[0], rhosrcrad);
    if ( !rcv->isCluster ) {
        COLOR Rd = topLevelGalerkinElement(rcv->patch)->Rd;
        colorProduct(Rd, rhosrcrad, rhosrcrad);
    }

    threshold = GLOBAL_stochasticRaytracing_hierarchy.epsilon * colorMaximumComponent(GLOBAL_statistics_maxSelfEmittedPower);
    propagated_power = rcv->area * ff * colorMaximumComponent(rhosrcrad);
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
        propagated_power *= rcv->imp;
        if ( !rcv->isCluster ) {
            propagated_power *= monteCarloRadiosityElementScalarReflectance(rcv);
        }
    }

    return (propagated_power < threshold);
}

static REFINE_ACTION SubdivideLargest(LINK *link) {
    StochasticRadiosityElement *rcv = link->rcv;
    StochasticRadiosityElement *src = link->src;
    if ( rcv->area < GLOBAL_stochasticRaytracing_hierarchy.minimumArea && src->area < GLOBAL_stochasticRaytracing_hierarchy.minimumArea ) {
        return dontRefine;
    } else {
        return (rcv->area > src->area) ? subdivideReceiver : subdivideSource;
    }
}

/**
Well known power-based refinement oracle (Hanrahan'91, with importance
a la Smits'92 if GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven is true)
*/
REFINE_ACTION
powerOracle(LINK *link) {
    if ( selfLink(link)) {
        return subdivideReceiver;
    } else if ( linkInvolvingClusters(link) && !disjunctElements(link->rcv, link->src)) {
        return SubdivideLargest(link);
    } else if ( LowPowerLink(link)) {
        return dontRefine;
    } else {
        return SubdivideLargest(link);
    }
}

/**
Constructs a toplevel link for given toplevel surface elements
rcvtop and srctop: the result is a link between the toplevel
cluster containing the whole scene and itself if clustering is
enabled. If clustering is not enabled, a link between the
given toplevel surface elements is returned
*/
LINK
topLink(StochasticRadiosityElement *rcvtop, StochasticRadiosityElement *srctop) {
    StochasticRadiosityElement *rcv, *src;
    LINK link;

    if ( GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing && GLOBAL_stochasticRaytracing_hierarchy.clustering != NO_CLUSTERING ) {
        src = rcv = GLOBAL_stochasticRaytracing_hierarchy.topCluster;
    } else {
        src = srctop;
        rcv = rcvtop;
    }

    link.rcv = rcv;
    link.src = src;

    return link;
}

/**
Refines a toplevel link (constructed with TopLink() above). The
returned LINK structure contains pointers the admissable
elements and corresponding point coordinates for light transport.
rcvtop and srctop are toplevel surface elements containing the
endpoint and origin respectively of a line along which light is to
be transported. (ur,vr) and (us,vs) are the uniform parameters of
the endpoint and origin on the toplevel surface elements on input.
They will be replaced by the point parameters on the admissable elements
after refinement
(ur,vr) are the coordinates of the point on the receiver patch,
(us,vs) coordinates of the point on the source patch
*/
LINK *
hierarchyRefine(
    LINK *link,
    StochasticRadiosityElement *rcvTop,
    double *ur,
    double *vr,
    StochasticRadiosityElement *srcTop,
    double *us,
    double *vs,
    ORACLE evaluate_link)
{
    if ( !GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing ) {
        link->rcv = rcvTop;
        link->src = srcTop;
    } else {
        REFINE_ACTION action;
        while ((action = evaluate_link(link)) != dontRefine ) {
            link = action(link, rcvTop, ur, vr, srcTop, us, vs);
        }
    }
    return link;
}
