#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "skin/Geometry.h"
#include "scene/scene.h"
#include "skin/Patch.h"
#include "GALERKIN/Shaft.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/galerkinP.h"
#include "GALERKIN/formfactor.h"

static GalerkinElement *globalElement; // The element for which initial links are to be created
static GalerkinRole globalRole; // The role of that element: SOURCE or RECEIVER
static Patch *globalPatch; // The patch for the element is the toplevel element
static BoundingBox globalPatchBoundingBox; // Bounding box for that patch
static java::ArrayList<Geometry *> *globalCandidateList; // Candidate list for shaft culling

static void
createInitialLink(Patch *patch) {
    GalerkinElement *rcv = nullptr;
    GalerkinElement *src = nullptr;
    java::ArrayList<Geometry *> *oldCandidateList = globalCandidateList;
    Interaction link{};
    float ff[MAX_BASIS_SIZE * MAX_BASIS_SIZE];
    link.K.p = ff;

    if ( !facing(patch, globalPatch)) {
        return;
    }

    GalerkinElement *topLevelElement = patchGalerkinElement(patch);
    switch ( globalRole ) {
        case SOURCE:
            rcv = topLevelElement;
            src = globalElement;
            break;
        case RECEIVER:
            rcv = globalElement;
            src = topLevelElement;
            break;
        default:
            logFatal(2, "createInitialLink", "Impossible element role");
    }

    if ( (GLOBAL_galerkin_state.exact_visibility || GLOBAL_galerkin_state.shaftCullMode == ALWAYS_DO_SHAFT_CULLING) && oldCandidateList ) {
        Shaft shaft;

        if ( GLOBAL_galerkin_state.exact_visibility ) {
            POLYGON rcvPolygon;
            POLYGON srcPolygon;
            shaft.constructFromPolygonToPolygon(rcv->polygon(&rcvPolygon),
                                                src->polygon(&srcPolygon));
        } else {
            BoundingBox bbox;
            shaft.constructShaft(&globalPatchBoundingBox, patch->patchBounds(&bbox));
        }

        shaft.setShaftOmit(globalPatch);
        shaft.setShaftOmit(patch);
        java::ArrayList<Geometry*> *arr = new java::ArrayList<Geometry*>();
        shaft.doCulling(oldCandidateList, globalCandidateList);
        globalCandidateList = arr;

        if ( shaft.cut == true ) {
            // One patch causes full occlusion
            freeCandidateList(globalCandidateList);
            globalCandidateList = oldCandidateList;
            return;
        }
    }

    link.receiverElement = rcv;
    link.sourceElement = src;

    if ( rcv != nullptr ) {
        link.nrcv = rcv->basisSize;
    }

    if ( src != nullptr ) {
        link.nsrc = src->basisSize;
    }

    bool isSceneGeometry = (globalCandidateList == GLOBAL_scene_geometries);
    bool isClusteredGeometry = (globalCandidateList == GLOBAL_scene_clusteredGeometries);
    java::ArrayList<Geometry *> *geometryListReferences = globalCandidateList;
    areaToAreaFormFactor(&link, geometryListReferences, isSceneGeometry, isClusteredGeometry);

    if ( GLOBAL_galerkin_state.exact_visibility || GLOBAL_galerkin_state.shaftCullMode == ALWAYS_DO_SHAFT_CULLING ) {
        if ( oldCandidateList != globalCandidateList ) {
            freeCandidateList(globalCandidateList);
        }
        globalCandidateList = oldCandidateList;
    }

    if ( link.vis > 0 ) {
        Interaction *newLink = interactionDuplicate(&link);
        // Store interactions with the source patch for the progressive radiosity method
        // and with the receiving patch for gathering methods
        if ( GLOBAL_galerkin_state.iteration_method == SOUTH_WELL ) {
            src->interactions->add(newLink);
        } else {
            rcv->interactions->add(newLink);
        }
    }
}

/**
Yes ... we exploit the hierarchical structure of the scene during initial linking
*/
static void
geomLink(Geometry *geom) {
    Shaft shaft;
    java::ArrayList<Geometry *> *oldCandidateList = globalCandidateList;

    // Immediately return if the Geometry is bounded and behind the plane of the patch for which interactions are created
    if ( geom->bounded && geomBounds(geom).behindPlane(&globalPatch->normal, globalPatch->planeConstant) ) {
        return;
    }

    // If the geometry is bounded, do shaft culling, reducing the candidate list
    // which contains the possible occluder between a pair of patches for which
    // an initial link will need to be created
    if ( geom->bounded && oldCandidateList ) {
        shaft.constructShaft(&globalPatchBoundingBox, &geomBounds(geom));
        shaft.setShaftOmit(globalPatch);
        java::ArrayList<Geometry*> *arr = new java::ArrayList<Geometry*>();
        shaft.doCulling(oldCandidateList, arr);
        globalCandidateList = arr;
    }

    // If the Geometry is an aggregate, test each of its children GEOMs, if it
    // is a primitive, create an initial link with each patch it consists of
    if ( geomIsAggregate(geom)) {
        java::ArrayList<Geometry *> *geometryList = geomPrimListCopy(geom);
        for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
            geomLink(geometryList->get(i));
        }
        delete geometryList;
    } else {
        java::ArrayList<Patch *> *patchList = geomPatchArrayListReference(geom);
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
    if ( top->flags & IS_CLUSTER_MASK ) {
        logFatal(-1, "createInitialLinks", "cannot use this routine for cluster elements");
    }

    globalElement = top;
    globalRole = role;
    globalPatch = top->patch;
    globalPatch->patchBounds(&globalPatchBoundingBox);
    globalCandidateList = GLOBAL_scene_clusteredGeometries;

    for ( int i = 0; GLOBAL_scene_geometries != nullptr && i < GLOBAL_scene_geometries->size(); i++ ) {
        geomLink(GLOBAL_scene_geometries->get(i));

    }
}

/**
Creates an initial link between the given element and the top cluster
*/
void
createInitialLinkWithTopCluster(GalerkinElement *elem, GalerkinRole role) {
    GalerkinElement *rcv = nullptr;
    GalerkinElement *src = nullptr;
    Interaction *newLink;
    FloatOrPointer K;
    FloatOrPointer deltaK;
    float ff[MAX_BASIS_SIZE * MAX_BASIS_SIZE];
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
    if ( rcv != nullptr && src != nullptr ) {
        if ( rcv->basisSize * src->basisSize == 1 ) {
            K.f = 0.0;
        } else {
            K.p = ff;
            for ( i = 0; i < rcv->basisSize * src->basisSize; i++ ) {
                K.p[i] = 0.0;
            }
        }
        deltaK.f = HUGE; // HUGE error on the form factor
    }

    newLink = new Interaction(
        rcv,
        src,
        K,
        deltaK,
        rcv->basisSize,
        src->basisSize,
        1,
        128
    );

    // Store interactions with the source patch for the progressive radiosity method
    // and with the receiving patch for gathering methods
    if ( GLOBAL_galerkin_state.iteration_method == SOUTH_WELL ) {
        src->interactions->add(newLink);
    } else {
        rcv->interactions->add(newLink);
    }
}
