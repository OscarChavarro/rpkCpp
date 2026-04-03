#include "common/RenderOptions.h"

/**
Hierarchical element refinement.

References:
[HANR1991] Hanrahan, Pat. et al, "A rapid hierarchical radiosity algorithm", SIGGRAPH 1991
[SMIT1992] Smits, Brian. et al, "An importance-driven radiosity algorithm", SIGGRAPH 1992
*/


#ifdef RAYTRACING_ENABLED
#include "common/RenderOptions.h"
#include "common/statistics/Statistics.h"
#include "raycasting/stochasticRaytracing/McradP.h"
#include "raycasting/stochasticRaytracing/Hierarchy.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

/**
Refinement action 1: do nothing (link is accurate enough)
Special refinement action used to indicate that a link is admissible
*/
Link *
Hierarchy::dontRefineCallBack(
    Link *link,
    StochasticRadiosityElement * /*rcvtop*/,
    double * /*ur*/,
    double * /*vr*/,
    StochasticRadiosityElement * /*srctop*/,
    double * /*us*/,
    double * /*vs*/,
    const RenderOptions * /*renderOptions*/)
{
    // Doesn't do anything
    return link;
}

/**
Refinement action 2: subdivide the receiver using regular quadtree subdivision
*/
Link *
Hierarchy::subdivideReceiverCallBack(
    Link *link,
    StochasticRadiosityElement *rcvtop,
    double *ur,
    double *vr,
    StochasticRadiosityElement * /*srctop*/,
    double * /*us*/,
    double * /*vs*/,
    const RenderOptions *renderOptions)
{
    StochasticRadiosityElement *rcv = link->rcv;
    if ( rcv->isCluster() ) {
        rcv = static_cast<StochasticRadiosityElement *>(rcv->childContainingElement(rcvtop));
    } else {
        if ( !rcv->regularSubElements ) {
            StochasticRadiosityElement::stochasticRadiosityElementRegularSubdivideElement(rcv, renderOptions);
        }
        rcv = StochasticRadiosityElement::stochasticRadiosityElementRegularSubElementAtPoint(rcv, ur, vr);
    }
    link->rcv = rcv;
    return link;
}

/**
Refinement action 3: subdivide the source using regular quadtree subdivision
*/
Link *
Hierarchy::subdivideSourceCallBack(
    Link *link,
    StochasticRadiosityElement * /*rcvtop*/,
    double * /*ur*/,
    double * /*vr*/,
    StochasticRadiosityElement *srcTop,
    double *us,
    double *vs,
    const RenderOptions *renderOptions)
{
    StochasticRadiosityElement *src = link->src;
    if ( src->isCluster() ) {
        src = static_cast<StochasticRadiosityElement *>(src->childContainingElement(srcTop));
    } else {
        if ( !src->regularSubElements ) {
            StochasticRadiosityElement::stochasticRadiosityElementRegularSubdivideElement(src, renderOptions);
        }
        src = StochasticRadiosityElement::stochasticRadiosityElementRegularSubElementAtPoint(src, us, vs);
    }
    link->src = src;
    return link;
}

bool
Hierarchy::selfLink(const Link *link) {
    return (link->rcv == link->src);
}

/**
Cheap form factor estimate to drive hierarchical refinement
as in Hanrahan SIGGRAPH'91
*/
float
Hierarchy::formFactorEstimate(const StochasticRadiosityElement *rcv, const StochasticRadiosityElement *src) {
    Vector3D D;
    D.subtraction(src->midPoint, rcv->midPoint);

    double d = D.norm();
    double f = src->area / (M_PI * d * d + src->area);
    double f2 = 2.0 * f;
    double c1 = rcv->isCluster() ? 1.0 /*0.25*/ : java::Math::abs(D.dotProduct(rcv->patch->normal)) / d;
    if ( c1 < f2 ) {
        c1 = f2;
    }
    double c2 = src->isCluster() ? 1.0 /*0.25*/ : java::Math::abs(D.dotProduct(src->patch->normal)) / d;
    if ( c2 < f2 ) {
        c2 = f2;
    }
    return static_cast<float>(f * c1 * c2);
}

bool
Hierarchy::lowPowerLink(
    const Link *link,
    const Statistics *statistics)
{
    const StochasticRadiosityElement *rcv = link->rcv;
    const StochasticRadiosityElement *src = link->src;
    ColorRgb rhoSrcRad;
    float ff = formFactorEstimate(rcv, src);
    float threshold;
    float propagatedPower;

    // Compute receiver reflectance times source radiosity
    rhoSrcRad.scaledCopy(static_cast<float>(M_PI), src->radiance[0]);
    if ( !rcv->isCluster() ) {
        ColorRgb Rd = McradP::topLevelStochasticRadiosityElement(rcv->patch)->Rd;
        rhoSrcRad.selfScalarProduct(Rd);
    }

    threshold = ElementHierarchyState::activeState().epsilon * statistics->radiance.maxSelfEmittedPower.maximumComponent();
    propagatedPower = rcv->area * ff * rhoSrcRad.maximumComponent();
    if ( StochasticRelaxation::activeState().importanceDriven ) {
        propagatedPower *= rcv->importance;
        if ( !rcv->isCluster() ) {
            propagatedPower *= StochasticRadiosityElement::stochasticRadiosityElementScalarReflectance(rcv);
        }
    }

    return (propagatedPower < threshold);
}

REFINE_ACTION
Hierarchy::subDivideLargest(const Link *link) {
    const StochasticRadiosityElement *rcv = link->rcv;
    const StochasticRadiosityElement *src = link->src;
    if ( rcv->area < ElementHierarchyState::activeState().minimumArea && src->area < ElementHierarchyState::activeState().minimumArea ) {
        return static_cast<REFINE_ACTION>(Hierarchy::dontRefineCallBack);
    } else {
        return (rcv->area > src->area) ? Hierarchy::subdivideReceiverCallBack : Hierarchy::subdivideSourceCallBack;
    }
}

/**
Well known power-based refinement oracle ([HANR1992] Hanrahan'91, with importance
a la [SMIT1992] Smits'92 when importance-driven sampling is enabled)
*/
REFINE_ACTION
Hierarchy::powerOracle(const Link *link) {
    if ( selfLink(link) ) {
        return static_cast<REFINE_ACTION>(Hierarchy::subdivideReceiverCallBack);
    } else if ( lowPowerLink(link, &Statistics::instance()) ) {
        return static_cast<REFINE_ACTION>(Hierarchy::dontRefineCallBack);
    } else {
        return subDivideLargest(link);
    }
}

/**
Constructs a toplevel link for given toplevel surface elements
rcvTop and srcTop: the result is a link between the toplevel
cluster containing the whole scene and itself if clustering is
enabled. If clustering is not enabled, a link between the
given toplevel surface elements is returned
*/
Link
Hierarchy::topLink(StochasticRadiosityElement *rcvTop, StochasticRadiosityElement *srcTop) {
    StochasticRadiosityElement *rcv;
    StochasticRadiosityElement *src;
    Link link{};

    if ( ElementHierarchyState::activeState().do_h_meshing
      && ElementHierarchyState::activeState().clustering != HierarchyClusteringMode::NO_CLUSTERING ) {
        src = rcv = ElementHierarchyState::activeState().topCluster;
    } else {
        src = srcTop;
        rcv = rcvTop;
    }

    link.rcv = rcv;
    link.src = src;

    return link;
}

/**
Refines a toplevel link (constructed with TopLink() above). The
returned Link structure contains pointers the admissible
elements and corresponding point coordinates for light transport.
rcvTop and srcTop are toplevel surface elements containing the
endpoint and origin respectively of a line along which light is to
be transported. (ur,vr) and (us,vs) are the uniform parameters of
the endpoint and origin on the toplevel surface elements on input.
They will be replaced by the point parameters on the admissible elements
after refinement
(ur,vr) are the coordinates of the point on the receiver patch,
(us,vs) coordinates of the point on the source patch
*/
Link *
Hierarchy::hierarchyRefine(
    Link *link,
    StochasticRadiosityElement *rcvTop,
    double *ur,
    double *vr,
    StochasticRadiosityElement *srcTop,
    double *us,
    double *vs,
    ORACLE evaluateLink,
    const RenderOptions *renderOptions)
{
    if ( !ElementHierarchyState::activeState().do_h_meshing ) {
        link->rcv = rcvTop;
        link->src = srcTop;
    } else {
        REFINE_ACTION action;
        while ( (action = evaluateLink(link)) != static_cast<REFINE_ACTION>(Hierarchy::dontRefineCallBack) ) {
            link = action(link, rcvTop, ur, vr, srcTop, us, vs, renderOptions);
        }
    }
    return link;
}

#endif
