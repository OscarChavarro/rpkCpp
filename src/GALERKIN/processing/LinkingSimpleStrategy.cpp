#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "GALERKIN/Shaft.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/processing/FormFactorStrategy.h"
#include "GALERKIN/processing/LinkingSimpleStrategy.h"

static void
createInitialLink(
    const Scene *scene,
    const GalerkinState *galerkinState,
    const GalerkinRole role,
    java::ArrayList<Geometry *> **candidateList,
    GalerkinElement *topElement,
    BoundingBox *topLevelBoundingBox,
    Patch *patch)
{
    if ( !patch->facing(topElement->patch) ) {
        return;
    }

    GalerkinElement *rcv = nullptr;
    GalerkinElement *src = nullptr;
    GalerkinElement *topLevelElement = galerkinGetElement(patch);
    switch ( role ) {
        case SOURCE:
            rcv = topLevelElement;
            src = topElement;
            break;
        case RECEIVER:
            rcv = topElement;
            src = topLevelElement;
            break;
        default:
            logFatal(2, "createInitialLink", "Impossible element role");
    }

    java::ArrayList<Geometry *> *oldCandidateList = *candidateList;

    if ( (galerkinState->exactVisibility || galerkinState->shaftCullMode == ALWAYS_DO_SHAFT_CULLING) && oldCandidateList ) {
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
            shaft.constructFromBoundingBoxes(topLevelBoundingBox, &boundingBox);
        }

        shaft.setShaftOmit(topElement->patch);
        shaft.setShaftOmit(patch);
        java::ArrayList<Geometry*> *arr = new java::ArrayList<Geometry*>();
        shaft.doCulling(oldCandidateList, *candidateList);
        *candidateList = arr;

        if ( shaft.cut == true ) {
            // One patch causes full occlusion
            freeCandidateList(*candidateList);
            *candidateList = oldCandidateList;
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

    bool isSceneGeometry = (*candidateList == scene->geometryList);
    bool isClusteredGeometry = (*candidateList == scene->clusteredGeometryList);
    const java::ArrayList<Geometry *> *geometryListReferences = *candidateList;
    FormFactorStrategy::computeAreaToAreaFormFactorVisibility(
        scene->voxelGrid,
        geometryListReferences,
        isSceneGeometry,
        isClusteredGeometry,
        &link,
        galerkinState);

    if ( galerkinState->exactVisibility || galerkinState->shaftCullMode == ALWAYS_DO_SHAFT_CULLING ) {
        if ( oldCandidateList != *candidateList ) {
            freeCandidateList(*candidateList);
        }
        *candidateList = oldCandidateList;
    }

    if ( link.visibility > 0 ) {
        Interaction *newLink = Interaction::interactionDuplicate(&link);
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
Yes... we exploit the hierarchical structure of the scene during initial linking
*/
static void
geometryLink(
    const Scene *scene,
    const GalerkinState *galerkinState,
    const GalerkinRole role,
    java::ArrayList<Geometry *> **candidateList,
    GalerkinElement *topElement,
    BoundingBox *topLevelBoundingBox,
    Geometry *geometry)
{
    Shaft shaft;
    java::ArrayList<Geometry *> *oldCandidateList = *candidateList;

    // Immediately return if the Geometry is bounded and behind the plane of the patch for which interactions are created
    if ( geometry->bounded && geometry->getBoundingBox().behindPlane(&topElement->patch->normal, topElement->patch->planeConstant) ) {
        return;
    }

    // If the geometry is bounded, do shaft culling, reducing the candidate list
    // which contains the possible occluder between a pair of patches for which
    // an initial link will need to be created
    if ( geometry->bounded && oldCandidateList ) {
        BoundingBox boundingBox = geometry->getBoundingBox();
        shaft.constructFromBoundingBoxes(topLevelBoundingBox, &boundingBox);
        shaft.setShaftOmit(topElement->patch);
        java::ArrayList<Geometry*> *arr = new java::ArrayList<Geometry*>();
        shaft.doCulling(oldCandidateList, arr);
        *candidateList = arr;
    }

    // If the Geometry is an aggregate, test each of its children GEOMs, if it
    // is a primitive, create an initial link with each patch it consists of
    if ( geometry->isCompound() ) {
        java::ArrayList<Geometry *> *geometryList = geomPrimListCopy(geometry);
        for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
            geometryLink(
                scene,
                galerkinState,
                role,
                candidateList,
                topElement,
                topLevelBoundingBox,
                geometryList->get(i));
        }
        delete geometryList;
    } else {
        const java::ArrayList<Patch *> *patchList = geomPatchArrayListReference(geometry);
        for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
            createInitialLink(
                scene,
                galerkinState,
                role,
                candidateList,
                topElement,
                topLevelBoundingBox,
                patchList->get(i));
        }
    }

    if ( geometry->bounded && oldCandidateList ) {
        freeCandidateList(*candidateList);
    }
    *candidateList = oldCandidateList;
}

/**
Creates the initial interactions for a toplevel element which is
considered to be a SOURCE or RECEIVER according to 'role'. Interactions
are stored at the receiver element when doing gathering and at the
source element when doing shooting
*/
void
LinkingSimpleStrategy::createInitialLinks(
    const Scene *scene,
    const GalerkinRole role,
    const GalerkinState *galerkinState,
    GalerkinElement *topElement)
{
    if ( topElement->flags & IS_CLUSTER_MASK ) {
        logFatal(-1, "createInitialLinks", "cannot use this routine for cluster elements");
    }

    BoundingBox topLevelBoundingBox;
    topElement->patch->getBoundingBox(&topLevelBoundingBox);

    java::ArrayList<Geometry *> *globalCandidateList = scene->clusteredGeometryList;

    for ( int i = 0; scene->geometryList != nullptr && i < scene->geometryList->size(); i++ ) {
        geometryLink(
            scene,
            galerkinState,
            role,
            &globalCandidateList,
            topElement,
            &topLevelBoundingBox,
            scene->geometryList->get(i));
    }
}
