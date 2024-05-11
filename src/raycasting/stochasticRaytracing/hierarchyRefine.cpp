/**
Hierarchical element refinement.

References:
Hanrahan, Pat. et al, "A rapid hierarchical radiosity algorithm", SIGGRAPH 1991
Smits, Brian. et al, "An importance-driven radiosity algorithm", SIGGRAPH 1992
*/

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "common/Statistics.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"

/**
Refinement action 1: do nothing (link is accurate enough)
Special refinement action used to indicate that a link is admissible
*/
static LINK *
dontRefine(
    LINK *link,
    StochasticRadiosityElement * /*rcvtop*/,
    double * /*ur*/,
    double * /*vr*/,
    StochasticRadiosityElement * /*srctop*/,
    double * /*us*/,
    double * /*vs*/,
    RenderOptions * /*renderOptions*/)
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
    StochasticRadiosityElement * /*srctop*/,
    double * /*us*/,
    double * /*vs*/,
    RenderOptions *renderOptions)
{
    StochasticRadiosityElement *rcv = link->rcv;
    if ( rcv->isCluster() ) {
        rcv = (StochasticRadiosityElement *)rcv->childContainingElement(rcvtop);
    } else {
        if ( !rcv->regularSubElements ) {
            stochasticRadiosityElementRegularSubdivideElement(rcv, renderOptions);
        }
        rcv = stochasticRadiosityElementRegularSubElementAtPoint(rcv, ur, vr);
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
    StochasticRadiosityElement * /*rcvtop*/,
    double * /*ur*/,
    double * /*vr*/,
    StochasticRadiosityElement *srcTop,
    double *us,
    double *vs,
    RenderOptions *renderOptions)
{
    StochasticRadiosityElement *src = link->src;
    if ( src->isCluster() ) {
        src = (StochasticRadiosityElement *)src->childContainingElement(srcTop);
    } else {
        if ( !src->regularSubElements ) {
            stochasticRadiosityElementRegularSubdivideElement(src, renderOptions);
        }
        src = stochasticRadiosityElementRegularSubElementAtPoint(src, us, vs);
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
    return link->rcv->isCluster() || link->src->isCluster();
}

static int
disjointElements(StochasticRadiosityElement *rcv, StochasticRadiosityElement *src) {
    BoundingBox receiveBounds{};
    BoundingBox sourceBounds{};

    stochasticRadiosityElementBounds(rcv, &receiveBounds);
    stochasticRadiosityElementBounds(src, &sourceBounds);
    return receiveBounds.disjointToOtherBoundingBox(&sourceBounds);
}

/**
Cheap form factor estimate to drive hierarchical refinement
as in Hanrahan SIGGRAPH'91
*/
static float
formFactorEstimate(const StochasticRadiosityElement *rcv, const StochasticRadiosityElement *src) {
    Vector3D D;
    D.subtraction(src->midPoint, rcv->midPoint);

    double d = vectorNorm(D);
    double f = src->area / (M_PI * d * d + src->area);
    double f2 = 2.0 * f;
    double c1 = rcv->isCluster() ? 1.0 /*0.25*/ : std::fabs(D.dotProduct(rcv->patch->normal)) / d;
    if ( c1 < f2 ) {
        c1 = f2;
    }
    double c2 = src->isCluster() ? 1.0 /*0.25*/ : std::fabs(D.dotProduct(src->patch->normal)) / d;
    if ( c2 < f2 ) {
        c2 = f2;
    }
    return (float)(f * c1 * c2);
}

static int
LowPowerLink(
    LINK *link,
    const Statistics *statistics)
{
    StochasticRadiosityElement *rcv = link->rcv;
    const StochasticRadiosityElement *src = link->src;
    ColorRgb rhoSrcRad;
    float ff = formFactorEstimate(rcv, src);
    float threshold;
    float propagatedPower;

    // Compute receiver reflectance times source radiosity
    rhoSrcRad.scaledCopy((float)M_PI, src->radiance[0]);
    if ( !rcv->isCluster() ) {
        ColorRgb Rd = topLevelStochasticRadiosityElement(rcv->patch)->Rd;
        rhoSrcRad.selfScalarProduct(Rd);
    }

    threshold = GLOBAL_stochasticRaytracing_hierarchy.epsilon * statistics->maxSelfEmittedPower.maximumComponent();
    propagatedPower = rcv->area * ff * rhoSrcRad.maximumComponent();
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
        propagatedPower *= rcv->importance;
        if ( !rcv->isCluster() ) {
            propagatedPower *= stochasticRadiosityElementScalarReflectance(rcv);
        }
    }

    return (propagatedPower < threshold);
}

static REFINE_ACTION subDivideLargest(const LINK *link) {
    const StochasticRadiosityElement *rcv = link->rcv;
    const StochasticRadiosityElement *src = link->src;
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
    if ( selfLink(link) ) {
        return subdivideReceiver;
    } else if ( linkInvolvingClusters(link) && !disjointElements(link->rcv, link->src) ) {
        return subDivideLargest(link);
    } else if ( LowPowerLink(link, &GLOBAL_statistics) ) {
        return dontRefine;
    } else {
        return subDivideLargest(link);
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
    LINK link{};

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
    ORACLE evaluateLink,
    RenderOptions *renderOptions)
{
    if ( !GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing ) {
        link->rcv = rcvTop;
        link->src = srcTop;
    } else {
        REFINE_ACTION action;
        while ((action = evaluateLink(link)) != dontRefine ) {
            link = action(link, rcvTop, ur, vr, srcTop, us, vs, renderOptions);
        }
    }
    return link;
}

#endif
