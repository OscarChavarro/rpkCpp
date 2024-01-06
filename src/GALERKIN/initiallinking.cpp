#include "common/error.h"
#include "skin/Geometry.h"
#include "scene/scene.h"
#include "skin/Patch.h"
#include "GALERKIN/shaftculling.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/galerkinP.h"
#include "GALERKIN/formfactor.h"

static ELEMENT *the_element;    /* the element for which initial links are to be created */
static ROLE the_role;        /* the role of that element: SOURCE or RECEIVER */
static PATCH *the_patch;    /* the patch for the element is the toplevel element */
static BOUNDINGBOX the_patch_bounds; /* bounding box for that patch */
static GeometryListNode *the_candlist;    /* candidate list for shaft culling */

void CreateInitialLink(PATCH *patch) {
    ELEMENT *rcv = (ELEMENT *) nullptr, *src = (ELEMENT *) nullptr;
    GeometryListNode *oldcandlist = the_candlist;
    INTERACTION link;
    float ff[MAXBASISSIZE * MAXBASISSIZE];
    link.K.p = ff;
    /* link.deltaK.p = */

    if ( !Facing(patch, the_patch)) {
        return;
    }

    switch ( the_role ) {
        case SOURCE:
            rcv = TOPLEVEL_ELEMENT(patch);
            src = the_element;
            break;
        case RECEIVER:
            rcv = the_element;
            src = TOPLEVEL_ELEMENT(patch);
            break;
        default:
            logFatal(2, "CreateInitialLink", "Impossible element role");
    }

    if ((GLOBAL_galerkin_state.exact_visibility || GLOBAL_galerkin_state.shaftcullmode == ALWAYS_DO_SHAFTCULLING) && oldcandlist ) {
        SHAFT shaft, *the_shaft;

        if ( GLOBAL_galerkin_state.exact_visibility ) {
            POLYGON rcvpoly, srcpoly;
            the_shaft = ConstructPolygonToPolygonShaft(ElementPolygon(rcv, &rcvpoly),
                                                       ElementPolygon(src, &srcpoly),
                                                       &shaft);
        } else {
            BOUNDINGBOX bbox;
            the_shaft = ConstructShaft(the_patch_bounds, PatchBounds(patch, bbox), &shaft);
        }

        if ( the_shaft ) {
            ShaftOmit(&shaft, (Geometry *) the_patch);
            ShaftOmit(&shaft, (Geometry *) patch);
            the_candlist = DoShaftCulling(oldcandlist, the_shaft, (GeometryListNode *) nullptr);

            if ( the_shaft->cut == true ) {    /* one patch causes full occlusion. */
                FreeCandidateList(the_candlist);
                the_candlist = oldcandlist;
                return;
            }
        } else {
            /* should never happen though */
            logWarning("CreateInitialLinks", "Unable to construct a shaft for shaft culling");
        }
    }

    link.rcv = rcv;
    link.src = src;
    link.nrcv = rcv->basis_size;
    link.nsrc = src->basis_size;
    AreaToAreaFormFactor(&link, the_candlist);

    if ( GLOBAL_galerkin_state.exact_visibility || GLOBAL_galerkin_state.shaftcullmode == ALWAYS_DO_SHAFTCULLING ) {
        if ( oldcandlist != the_candlist ) {
            FreeCandidateList(the_candlist);
        }
        the_candlist = oldcandlist;
    }

    if ( link.vis > 0 ) {
        /* store interactions with the source patch for the progressive radiosity method
         * and with the receiving patch for gathering mathods. */
        if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL ) {
            src->interactions = InteractionListAdd(src->interactions, InteractionDuplicate(&link));
        } else {
            rcv->interactions = InteractionListAdd(rcv->interactions, InteractionDuplicate(&link));
        }
    }
}

/* yes ... we exploit the hierarchical structure of the scene during initial linking */
static void GeomLink(Geometry *geom) {
    SHAFT shaft;
    GeometryListNode *oldCandList = the_candlist;

    /* immediately return if the Geometry is bounded and behind the plane of the patch
     * for which itneractions are created ... */
    if ( geom->bounded && BoundsBehindPlane(geomBounds(geom), &the_patch->normal, the_patch->plane_constant)) {
        return;
    }

    /* if the geometry is bounded, do shaft culling, reducing the candidate list
     * which contains the possible occluders between a pair of patches for which
     * an initial link will need to be created. */
    if ( geom->bounded && oldCandList ) {
        ConstructShaft(the_patch_bounds, geomBounds(geom), &shaft);
        ShaftOmit(&shaft, (Geometry *) the_patch);
        the_candlist = DoShaftCulling(oldCandList, &shaft, nullptr);
    }

    /* if the Geometry is an aggregate, test each of it's childer GEOMs, if it
    * is a primitive, create an initial link with each patch it consists of. */
    if ( geomIsAggregate(geom)) {
        GeometryListNode *geoml = geomPrimList(geom);
        GeomListIterate(geoml, GeomLink);
    } else {
        PatchSet *patchl = geomPatchList(geom);
        PatchListIterate(patchl, CreateInitialLink);
    }

    if ( geom->bounded && oldCandList ) {
        FreeCandidateList(the_candlist);
    }
    the_candlist = oldCandList;
}

/* Creates the initial interactions for a toplevel element which is
 * considered to be a SOURCE or RECEIVER according to 'role'. Interactions
 * are stored at the receiver element when doing gathering and at the
 * source element when doing shooting. */
void CreateInitialLinks(ELEMENT *top, ROLE role) {
    if ( IsCluster(top)) {
        logFatal(-1, "CreateInitialLinks", "cannot use this routine for cluster elements");
    }

    the_element = top;
    the_role = role;
    the_patch = top->pog.patch;
    PatchBounds(the_patch, the_patch_bounds);
    the_candlist = GLOBAL_scene_clusteredWorld;
    GeomListIterate(GLOBAL_scene_world, GeomLink);
}

/* Creates an initial link between the given element and the top cluster. */
void CreateInitialLinkWithTopCluster(ELEMENT *elem, ROLE role) {
    ELEMENT *rcv = (ELEMENT *) nullptr, *src = (ELEMENT *) nullptr;
    INTERACTION *link;
    FloatOrPointer K, deltaK;
    float ff[MAXBASISSIZE * MAXBASISSIZE];
    int i;

    switch ( role ) {
        case RECEIVER:
            rcv = elem;
            src = GLOBAL_galerkin_state.top_cluster;
            break;
        case SOURCE:
            src = elem;
            rcv = GLOBAL_galerkin_state.top_cluster;
            break;
        default:
            logFatal(-1, "CreateInitialLinkWithTopCluster", "Invalid role");
    }

    /* assume no light transport (overlapping receiver and source) */
    if ( rcv->basis_size * src->basis_size == 1 ) {
        K.f = 0.;
    } else {
        K.p = ff;
        for ( i = 0; i < rcv->basis_size * src->basis_size; i++ ) {
            K.p[i] = 0.;
        }
    }
    deltaK.f = HUGE;    /* HUGE error on the form factor */

    link = InteractionCreate(rcv, src,
                             K, deltaK,
                             rcv->basis_size /*nrcv*/,
                             src->basis_size /*nsrc*/,
            /*crcv*/ 1, /*vis*/ 128
    );

    /* store interactions with the source patch for the progressive radiosity method
     * and with the receiving patch for gathering mathods. */
    if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL ) {
        src->interactions = InteractionListAdd(src->interactions, link);
    } else {
        rcv->interactions = InteractionListAdd(rcv->interactions, link);
    }
}


