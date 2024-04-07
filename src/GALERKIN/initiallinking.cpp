#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/mymath.h"
#include "scene/scene.h"
#include "GALERKIN/Shaft.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/formfactor.h"
#include "GALERKIN/initiallinking.h"
#include "GALERKIN/GalerkinState.h"

static GalerkinElement *globalElement; // The element for which initial links are to be created
static GalerkinRole globalRole; // The role of that element: SOURCE or RECEIVER
static Patch *globalPatch; // The patch for the element is the toplevel element
static BoundingBox globalPatchBoundingBox; // Bounding box for that patch
static java::ArrayList<Geometry *> *globalCandidateList; // Candidate list for shaft culling

static void
createInitialLink(Patch *patch, GalerkinState *galerkinState) {
    if ( !facing(patch, globalPatch) ) {
        return;
    }

    GalerkinElement *rcv = nullptr;
    GalerkinElement *src = nullptr;
    GalerkinElement *topLevelElement = galerkinGetElement(patch);
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

    java::ArrayList<Geometry *> *oldCandidateList = globalCandidateList;

    if ((galerkinState->exactVisibility || galerkinState->shaftCullMode == ALWAYS_DO_SHAFT_CULLING) && oldCandidateList ) {
        Shaft shaft;

        if ( galerkinState->exactVisibility ) {
            if ( rcv != nullptr && src != nullptr ) {
                Polygon rcvPolygon;
                Polygon srcPolygon;
                rcv->initPolygon(&rcvPolygon);
                src->initPolygon(&srcPolygon);
                shaft.constructFromPolygonToPolygon(&rcvPolygon, &srcPolygon);
            }
        } else {
            BoundingBox boundingBox;
            patch->getBoundingBox(&boundingBox);
            shaft.constructShaft(&globalPatchBoundingBox, &boundingBox);
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

    Interaction link{};
    link.K = new float[MAX_BASIS_SIZE * MAX_BASIS_SIZE];
    link.receiverElement = rcv;
    link.sourceElement = src;

    if ( rcv != nullptr ) {
        link.numberOfBasisFunctionsOnReceiver = rcv->basisSize;
    }

    if ( src != nullptr ) {
        link.numberOfBasisFunctionsOnSource = src->basisSize;
    }

    bool isSceneGeometry = (globalCandidateList == GLOBAL_scene_geometries);
    bool isClusteredGeometry = (globalCandidateList == GLOBAL_scene_clusteredGeometries);
    java::ArrayList<Geometry *> *geometryListReferences = globalCandidateList;
    areaToAreaFormFactor(&link, geometryListReferences, isSceneGeometry, isClusteredGeometry, galerkinState);

    if ( galerkinState->exactVisibility || galerkinState->shaftCullMode == ALWAYS_DO_SHAFT_CULLING ) {
        if ( oldCandidateList != globalCandidateList ) {
            freeCandidateList(globalCandidateList);
        }
        globalCandidateList = oldCandidateList;
    }

    if ( link.visibility > 0 ) {
        Interaction *newLink = interactionDuplicate(&link);
        // Store interactions with the source patch for the progressive radiosity method
        // and with the receiving patch for gathering methods
        if ( galerkinState->galerkinIterationMethod == SOUTH_WELL ) {
            if ( src != nullptr ) {
                src->interactions->add(newLink);
            }
        } else if ( rcv != nullptr ) {
            rcv->interactions->add(newLink);
        }
    }
}

/**
Yes ... we exploit the hierarchical structure of the scene during initial linking
*/
static void
geometryLink(Geometry *geometry, GalerkinState *galerkinState) {
    Shaft shaft;
    java::ArrayList<Geometry *> *oldCandidateList = globalCandidateList;

    // Immediately return if the Geometry is bounded and behind the plane of the patch for which interactions are created
    if ( geometry->bounded && getBoundingBox(geometry).behindPlane(&globalPatch->normal, globalPatch->planeConstant) ) {
        return;
    }

    // If the geometry is bounded, do shaft culling, reducing the candidate list
    // which contains the possible occluder between a pair of patches for which
    // an initial link will need to be created
    if ( geometry->bounded && oldCandidateList ) {
        shaft.constructShaft(&globalPatchBoundingBox, &getBoundingBox(geometry));
        shaft.setShaftOmit(globalPatch);
        java::ArrayList<Geometry*> *arr = new java::ArrayList<Geometry*>();
        shaft.doCulling(oldCandidateList, arr);
        globalCandidateList = arr;
    }

    // If the Geometry is an aggregate, test each of its children GEOMs, if it
    // is a primitive, create an initial link with each patch it consists of
    if ( geometry->isCompound() ) {
        java::ArrayList<Geometry *> *geometryList = geomPrimListCopy(geometry);
        for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
            geometryLink(geometryList->get(i), galerkinState);
        }
        delete geometryList;
    } else {
        java::ArrayList<Patch *> *patchList = geomPatchArrayListReference(geometry);
        for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
            createInitialLink(patchList->get(i), galerkinState);
        }
    }

    if ( geometry->bounded && oldCandidateList ) {
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
createInitialLinks(GalerkinElement *top, GalerkinRole role, GalerkinState *galerkinState) {
    if ( top->flags & IS_CLUSTER_MASK ) {
        logFatal(-1, "createInitialLinks", "cannot use this routine for cluster elements");
    }

    globalElement = top;
    globalRole = role;
    globalPatch = top->patch;
    globalPatch->getBoundingBox(&globalPatchBoundingBox);
    globalCandidateList = GLOBAL_scene_clusteredGeometries;

    for ( int i = 0; GLOBAL_scene_geometries != nullptr && i < GLOBAL_scene_geometries->size(); i++ ) {
        geometryLink(GLOBAL_scene_geometries->get(i), galerkinState);
    }
}

/**
Creates an initial link between the given element and the top cluster
*/
void
createInitialLinkWithTopCluster(GalerkinElement *elem, GalerkinRole role, GalerkinState *galerkinState) {
    GalerkinElement *rcv = nullptr;
    GalerkinElement *src = nullptr;

    switch ( role ) {
        case RECEIVER:
            rcv = elem;
            src = galerkinState->topCluster;
            break;
        case SOURCE:
            src = elem;
            rcv = galerkinState->topCluster;
            break;
        default:
            logFatal(-1, "createInitialLinkWithTopCluster", "Invalid role");
    }

    // Assume no light transport (overlapping receiver and source)
    float *K = nullptr;
    float *deltaK = nullptr;

    if ( rcv != nullptr && src != nullptr ) {
        if ( rcv->basisSize * src->basisSize == 1 ) {
            K = new float[1];
            K[0] = 0.0;
        } else {
            K = new float[MAX_BASIS_SIZE * MAX_BASIS_SIZE];
            for ( int i = 0; i < rcv->basisSize * src->basisSize; i++ ) {
                K[i] = 0.0;
            }
        }
        deltaK = new float[1];
        deltaK[0] = HUGE; // HUGE error on the form factor
    }

    Interaction *newLink = new Interaction(
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
    if ( galerkinState->galerkinIterationMethod == SOUTH_WELL ) {
        src->interactions->add(newLink);
    } else {
        rcv->interactions->add(newLink);
    }
}
