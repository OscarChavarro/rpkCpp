#include "common/error.h"
#include "skin/Geometry.h"
#include "scene/scene.h"
#include "skin/Patch.h"
#include "GALERKIN/shaftculling.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/galerkinP.h"
#include "GALERKIN/formfactor.h"

static ELEMENT *globalElement; // The element for which initial links are to be created
static ROLE globalRole; // The role of that element: SOURCE or RECEIVER
static Patch *globalPatch; // The patch for the element is the toplevel element
static BOUNDINGBOX globalPatchBoundingBox; // Bounding box for that patch
static GeometryListNode *globalCandidateList; // Candidate list for shaft culling

static void
createInitialLink(Patch *patch) {
    ELEMENT *rcv = nullptr;
    ELEMENT *src = nullptr;
    GeometryListNode *oldCandidateList = globalCandidateList;
    INTERACTION link{};
    float ff[MAXBASISSIZE * MAXBASISSIZE];
    link.K.p = ff;

    if ( !Facing(patch, globalPatch)) {
        return;
    }

    switch ( globalRole ) {
        case SOURCE:
            rcv = TOPLEVEL_ELEMENT(patch);
            src = globalElement;
            break;
        case RECEIVER:
            rcv = globalElement;
            src = TOPLEVEL_ELEMENT(patch);
            break;
        default:
            logFatal(2, "createInitialLink", "Impossible element role");
    }

    if ((GLOBAL_galerkin_state.exact_visibility || GLOBAL_galerkin_state.shaftcullmode == ALWAYS_DO_SHAFTCULLING) && oldCandidateList ) {
        SHAFT shaft, *the_shaft;

        if ( GLOBAL_galerkin_state.exact_visibility ) {
            POLYGON rcvpoly, srcpoly;
            the_shaft = ConstructPolygonToPolygonShaft(ElementPolygon(rcv, &rcvpoly),
                                                       ElementPolygon(src, &srcpoly),
                                                       &shaft);
        } else {
            BOUNDINGBOX bbox;
            the_shaft = ConstructShaft(globalPatchBoundingBox, patchBounds(patch, bbox), &shaft);
        }

        if ( the_shaft ) {
            ShaftOmit(&shaft, (Geometry *) globalPatch);
            ShaftOmit(&shaft, (Geometry *) patch);
            globalCandidateList = DoShaftCulling(oldCandidateList, the_shaft, (GeometryListNode *) nullptr);

            if ( the_shaft->cut == true ) {    /* one patch causes full occlusion. */
                FreeCandidateList(globalCandidateList);
                globalCandidateList = oldCandidateList;
                return;
            }
        } else {
            /* should never happen though */
            logWarning("createInitialLinks", "Unable to construct a shaft for shaft culling");
        }
    }

    link.rcv = rcv;
    link.src = src;
    link.nrcv = rcv->basis_size;
    link.nsrc = src->basis_size;
    AreaToAreaFormFactor(&link, globalCandidateList);

    if ( GLOBAL_galerkin_state.exact_visibility || GLOBAL_galerkin_state.shaftcullmode == ALWAYS_DO_SHAFTCULLING ) {
        if ( oldCandidateList != globalCandidateList ) {
            FreeCandidateList(globalCandidateList);
        }
        globalCandidateList = oldCandidateList;
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

/**
Yes ... we exploit the hierarchical structure of the scene during initial linking
*/
static void
geomLink(Geometry *geom) {
    SHAFT shaft;
    GeometryListNode *oldCandidateList = globalCandidateList;

    // Immediately return if the Geometry is bounded and behind the plane of the patch for which interactions are created
    if ( geom->bounded && boundsBehindPlane(geomBounds(geom), &globalPatch->normal, globalPatch->planeConstant)) {
        return;
    }

    /* if the geometry is bounded, do shaft culling, reducing the candidate list
     * which contains the possible occluders between a pair of patches for which
     * an initial link will need to be created. */
    if ( geom->bounded && oldCandidateList ) {
        ConstructShaft(globalPatchBoundingBox, geomBounds(geom), &shaft);
        ShaftOmit(&shaft, (Geometry *) globalPatch);
        globalCandidateList = DoShaftCulling(oldCandidateList, &shaft, nullptr);
    }

    // If the Geometry is an aggregate, test each of its children GEOMs, if it
    // is a primitive, create an initial link with each patch it consists of
    if ( geomIsAggregate(geom)) {
        GeometryListNode *geometryList = geomPrimList(geom);
        for ( GeometryListNode *window = geometryList; window != nullptr; window = window->next ) {
            Geometry *geometry = window->geom;
            geomLink(geometry);
        }
    } else {
        for ( PatchSet *window = geomPatchList(geom); window != nullptr; window = window->next ) {
            createInitialLink(window->patch);
        }
    }

    if ( geom->bounded && oldCandidateList ) {
        FreeCandidateList(globalCandidateList);
    }
    globalCandidateList = oldCandidateList;
}

/**
Creates the initial interactions for a toplevel element which is
considered to be a SOURCE or RECEIVER according to 'role'. Interactions
are stored at the receiver element when doing gathering and at the
source element when doing shooting
*/
void
createInitialLinks(ELEMENT *top, ROLE role) {
    if ( top->flags & IS_CLUSTER ) {
        logFatal(-1, "createInitialLinks", "cannot use this routine for cluster elements");
    }

    globalElement = top;
    globalRole = role;
    globalPatch = top->pog.patch;
    patchBounds(globalPatch, globalPatchBoundingBox);
    globalCandidateList = GLOBAL_scene_clusteredWorld;

    for ( GeometryListNode *window = (GLOBAL_scene_world); window != nullptr; window = window->next ) {
        Geometry *geometry = window->geom;
        geomLink(geometry);
    }
}

/**
Creates an initial link between the given element and the top cluster
*/
void
createInitialLinkWithTopCluster(ELEMENT *elem, ROLE role) {
    ELEMENT *rcv = (ELEMENT *) nullptr, *src = (ELEMENT *) nullptr;
    INTERACTION *link;
    FloatOrPointer K;
    FloatOrPointer deltaK;
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
            logFatal(-1, "createInitialLinkWithTopCluster", "Invalid role");
    }

    // Assume no light transport (overlapping receiver and source)
    if ( rcv->basis_size * src->basis_size == 1 ) {
        K.f = 0.0;
    } else {
        K.p = ff;
        for ( i = 0; i < rcv->basis_size * src->basis_size; i++ ) {
            K.p[i] = 0.0;
        }
    }
    deltaK.f = HUGE; // HUGE error on the form factor

    link = InteractionCreate(
        rcv,
        src,
        K,
        deltaK,
        rcv->basis_size, // nrcv
        src->basis_size, // nsrc
        1, // crcv
        128 // vis
    );

    // Store interactions with the source patch for the progressive radiosity method
    // and with the receiving patch for gathering methods
    if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL ) {
        src->interactions = InteractionListAdd(src->interactions, link);
    } else {
        rcv->interactions = InteractionListAdd(rcv->interactions, link);
    }
}
