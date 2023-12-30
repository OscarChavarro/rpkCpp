/* refine.c: hierarchical element refinement */

#include "material/statistics.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"

/* refinement action 1: do nothing (link is accurate enough) */
LINK *DontRefine(LINK *link,
                 ELEMENT *rcvtop, double *ur, double *vr,
                 ELEMENT *srctop, double *us, double *vs) {
    /* doesn't do anything */
    return link;
}

/* refinement action 2: subdivide the receiver using regular quadtree subdivision */
static LINK *SubdivideReceiver(LINK *link,
                               ELEMENT *rcvtop, double *ur, double *vr,
                               ELEMENT *srctop, double *us, double *vs) {
    ELEMENT *rcv = link->rcv;
    if ( rcv->iscluster ) {
        rcv = ClusterChildContainingElement(rcv, rcvtop);
    } else {
        if ( !rcv->regular_subelements ) {
            McrRegularSubdivideElement(rcv);
        }
        rcv = McrRegularSubelementAtPoint(rcv, ur, vr);
    }
    link->rcv = rcv;
    return link;
}

/* refinement action 3: subdivide the source using regular quadtree subdivision */
static LINK *SubdivideSource(LINK *link,
                             ELEMENT *rcvtop, double *ur, double *vr,
                             ELEMENT *srctop, double *us, double *vs) {
    ELEMENT *src = link->src;
    if ( src->iscluster ) {
        src = ClusterChildContainingElement(src, srctop);
    } else {
        if ( !src->regular_subelements ) {
            McrRegularSubdivideElement(src);
        }
        src = McrRegularSubelementAtPoint(src, us, vs);
    }
    link->src = src;
    return link;
}

static int SelfLink(LINK *link) {
    return (link->rcv == link->src);
}

static int LinkInvolvingClusters(LINK *link) {
    return (link->rcv->iscluster || link->src->iscluster);
}

static int DisjunctElements(ELEMENT *rcv, ELEMENT *src) {
    BOUNDINGBOX rcvbounds, srcbounds;
    ElementBounds(rcv, rcvbounds);
    ElementBounds(src, srcbounds);
    return DisjunctBounds(rcvbounds, srcbounds);
}

/* Cheap form factor estimate to drive hierarchical refinement 
 * as in Hanrahan SIGGRAPH'91. */
static float formfactor_estimate(ELEMENT *rcv, ELEMENT *src) {
    Vector3D D;
    double d, c1, c2, f, f2;
    VECTORSUBTRACT(src->midpoint, rcv->midpoint, D);
    d = VECTORNORM(D);
    f = src->area / (M_PI * d * d + src->area);
    f2 = 2. * f;
    c1 = rcv->iscluster ? 1. /*0.25*/ : fabs(VECTORDOTPRODUCT(D, rcv->pog.patch->normal)) / d;
    if ( c1 < f2 ) {
        c1 = f2;
    }
    c2 = src->iscluster ? 1. /*0.25*/ : fabs(VECTORDOTPRODUCT(D, src->pog.patch->normal)) / d;
    if ( c2 < f2 ) {
        c2 = f2;
    }
    return f * c1 * c2;
}

static int LowPowerLink(LINK *link) {
    ELEMENT *rcv = link->rcv;
    ELEMENT *src = link->src;
    COLOR rhosrcrad;
    float ff = formfactor_estimate(rcv, src);
    float threshold, propagated_power;

    /* compute receiver reflectance times source radiosity */
    COLORSCALE(M_PI, src->rad[0], rhosrcrad);
    if ( !rcv->iscluster ) {
        COLOR Rd = REFLECTANCE(rcv->pog.patch);
        COLORPROD(Rd, rhosrcrad, rhosrcrad);
    }

    threshold = hierarchy.epsilon * COLORMAXCOMPONENT(GLOBAL_statistics_maxSelfEmittedPower);
    propagated_power = rcv->area * ff * COLORMAXCOMPONENT(rhosrcrad);
    if ( mcr.importance_driven ) {
        propagated_power *= rcv->imp;
        if ( !rcv->iscluster ) {
            propagated_power *= ElementScalarReflectance(rcv);
        }
    }

    return (propagated_power < threshold);
}

static REFINE_ACTION SubdivideLargest(LINK *link) {
    ELEMENT *rcv = link->rcv;
    ELEMENT *src = link->src;
    if ( rcv->area < hierarchy.minarea && src->area < hierarchy.minarea ) {
        return DontRefine;
    } else {
        return (rcv->area > src->area) ? SubdivideReceiver : SubdivideSource;
    }
}

REFINE_ACTION PowerOracle(LINK *link) {
    if ( SelfLink(link)) {
        return SubdivideReceiver;
    } else if ( LinkInvolvingClusters(link) && !DisjunctElements(link->rcv, link->src)) {
        return SubdivideLargest(link);
    } else if ( LowPowerLink(link)) {
        return DontRefine;
    } else {
        return SubdivideLargest(link);
    }
}

LINK TopLink(ELEMENT *rcvtop, ELEMENT *srctop) {
    ELEMENT *rcv, *src;
    LINK link;

    if ( hierarchy.do_h_meshing && hierarchy.clustering != NO_CLUSTERING ) {
        src = rcv = hierarchy.topcluster;
    } else {
        src = srctop;
        rcv = rcvtop;
    }

    link.rcv = rcv;
    link.src = src;

    return link;
}

/* (ur,vr) are the coordinates of the point on the receiver patch, 
 * (us,vs) coordinates of the point on the source patch. */
LINK *Refine(LINK *link,
             ELEMENT *rcvtop, double *ur, double *vr,
             ELEMENT *srctop, double *us, double *vs,
             ORACLE evaluate_link) {
    if ( !hierarchy.do_h_meshing ) {
        link->rcv = rcvtop;
        link->src = srctop;
    } else {
        REFINE_ACTION action;
        while ((action = evaluate_link(link)) != DontRefine ) {
            link = action(link, rcvtop, ur, vr, srctop, us, vs);
        }
    }
    return link;
}
