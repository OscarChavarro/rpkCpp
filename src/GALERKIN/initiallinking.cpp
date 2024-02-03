#include "common/error.h"
#include "skin/Geometry.h"
#include "scene/scene.h"
#include "skin/Patch.h"
#include "GALERKIN/shaftculling.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/galerkinP.h"
#include "GALERKIN/formfactor.h"

static GalerkinElement *globalElement; // The element for which initial links are to be created
static GalerkinRole globalRole; // The role of that element: SOURCE or RECEIVER
static Patch *globalPatch; // The patch for the element is the toplevel element
static BOUNDINGBOX globalPatchBoundingBox; // Bounding box for that patch
static GeometryListNode *globalCandidateList; // Candidate list for shaft culling

static void
createInitialLink(Patch *patch) {
    GalerkinElement *rcv = nullptr;
    GalerkinElement *src = nullptr;
    GeometryListNode *oldCandidateList = globalCandidateList;
    INTERACTION link{};
    float ff[MAXBASISSIZE * MAXBASISSIZE];
    link.K.p = ff;

    if ( !facing(patch, globalPatch)) {
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

    if ((GLOBAL_galerkin_state.exact_visibility || GLOBAL_galerkin_state.shaftCullMode == ALWAYS_DO_SHAFT_CULLING) && oldCandidateList ) {
        SHAFT shaft, *the_shaft;

        if ( GLOBAL_galerkin_state.exact_visibility ) {
            POLYGON rcvpoly, srcpoly;
            the_shaft = constructPolygonToPolygonShaft(galerkinElementPolygon(rcv, &rcvpoly),
                                                       galerkinElementPolygon(src, &srcpoly),
                                                       &shaft);
        } else {
            BOUNDINGBOX bbox;
            the_shaft = constructShaft(globalPatchBoundingBox, patchBounds(patch, bbox), &shaft);
        }

        if ( the_shaft ) {
            setShaftOmit(&shaft, globalPatch);
            setShaftOmit(&shaft, patch);
            globalCandidateList = doShaftCulling(oldCandidateList, the_shaft, (GeometryListNode *) nullptr);

            if ( the_shaft->cut == true ) {    /* one patch causes full occlusion. */
                freeCandidateList(globalCandidateList);
                globalCandidateList = oldCandidateList;
                return;
            }
        } else {
            /* should never happen though */
            logWarning("createInitialLinks", "Unable to construct a shaft for shaft culling");
        }
    }

    link.receiverElement = rcv;
    link.sourceElement = src;
    link.nrcv = rcv->basisSize;
    link.nsrc = src->basisSize;
    areaToAreaFormFactor(&link, globalCandidateList);

    if ( GLOBAL_galerkin_state.exact_visibility || GLOBAL_galerkin_state.shaftCullMode == ALWAYS_DO_SHAFT_CULLING ) {
        if ( oldCandidateList != globalCandidateList ) {
            freeCandidateList(globalCandidateList);
        }
        globalCandidateList = oldCandidateList;
    }

    if ( link.vis > 0 ) {
        /* store interactions with the source patch for the progressive radiosity method
         * and with the receiving patch for gathering mathods. */
        if ( GLOBAL_galerkin_state.iteration_method == SOUTH_WELL ) {
            src->interactions = InteractionListAdd(src->interactions, interactionDuplicate(&link));
        } else {
            rcv->interactions = InteractionListAdd(rcv->interactions, interactionDuplicate(&link));
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
        constructShaft(globalPatchBoundingBox, geomBounds(geom), &shaft);
        setShaftOmit(&shaft, globalPatch);
        globalCandidateList = doShaftCulling(oldCandidateList, &shaft, nullptr);
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
        java::ArrayList<Patch *> *patchList = geomPatchArrayList(geom);
        for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
            createInitialLink(patchList->get(i));
        }
    }

    if ( geom->bounded && oldCandidateList ) {
        freeCandidateList(globalCandidateList);
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
createInitialLinks(GalerkinElement *top, GalerkinRole role) {
    if ( top->flags & IS_CLUSTER ) {
        logFatal(-1, "createInitialLinks", "cannot use this routine for cluster elements");
    }

    globalElement = top;
    globalRole = role;
    globalPatch = top->patch;
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
createInitialLinkWithTopCluster(GalerkinElement *elem, GalerkinRole role) {
    GalerkinElement *rcv = (GalerkinElement *) nullptr, *src = (GalerkinElement *) nullptr;
    INTERACTION *link;
    FloatOrPointer K;
    FloatOrPointer deltaK;
    float ff[MAXBASISSIZE * MAXBASISSIZE];
    int i;

    switch ( role ) {
        case RECEIVER:
            rcv = elem;
            src = GLOBAL_galerkin_state.topCluster;
            break;
        case SOURCE:
            src = elem;
            rcv = GLOBAL_galerkin_state.topCluster;
            break;
        default:
            logFatal(-1, "createInitialLinkWithTopCluster", "Invalid role");
    }

    // Assume no light transport (overlapping receiver and source)
    if ( rcv->basisSize * src->basisSize == 1 ) {
        K.f = 0.0;
    } else {
        K.p = ff;
        for ( i = 0; i < rcv->basisSize * src->basisSize; i++ ) {
            K.p[i] = 0.0;
        }
    }
    deltaK.f = HUGE; // HUGE error on the form factor

    link = interactionCreate(
            rcv,
            src,
            K,
            deltaK,
            rcv->basisSize, // nrcv
            src->basisSize, // nsrc
            1, // crcv
            128 // vis
    );

    // Store interactions with the source patch for the progressive radiosity method
    // and with the receiving patch for gathering methods
    if ( GLOBAL_galerkin_state.iteration_method == SOUTH_WELL ) {
        src->interactions = InteractionListAdd(src->interactions, link);
    } else {
        rcv->interactions = InteractionListAdd(rcv->interactions, link);
    }
}
