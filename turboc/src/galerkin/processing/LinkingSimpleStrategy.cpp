#include "java/util/ArrayList.txx"
#include "common/MemoryPool.txx"
#include "common/Error.h"
#include "galerkin/processing/FormFactorStrategy.h"
#include "galerkin/processing/LinkingSimpleStrategy.h"
#include "galerkin/Shaft.h"

static MemoryPool<float> gInteractionCoefficientsPool;
static bool gInteractionCoefficientsPoolInitialized = false;

static void ensureInteractionCoefficientsPool() {
    if ( !gInteractionCoefficientsPoolInitialized ) {
        gInteractionCoefficientsPool.init(8 * 1024 * 1024);
        gInteractionCoefficientsPoolInitialized = true;
    }
}

void
LinkingSimpleStrategy::createInitialLink(
    const Scene *scene,
    const GalerkinState *galerkinState,
    const GalerkinRole role,
    ArrayList<Geometry *> **candidateList,
    GalerkinElement *topElement,
    BoundingBox *topLevelBoundingBox,
    Patch *patch)
{
    if ( !patch->facing(topElement->patch) ) {
        return;
    }

    GalerkinElement *rcv = NULL;
    GalerkinElement *src = NULL;
    GalerkinElement *topLevelElement = GalerkinElement::fromPatch(patch);
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
            Error::fatal(2, "createInitialLink", "Impossible element role");
    }

    ArrayList<Geometry *> *oldCandidateList = *candidateList;

    if ( (galerkinState->exactVisibility
       || galerkinState->shaftCullMode == ALWAYS_DO_SHAFT_CULLING)
       && oldCandidateList ) {
        Shaft shaft;

        if ( galerkinState->exactVisibility ) {
            if ( rcv != NULL && src != NULL ) {
                Polygon rcvPolygon;
                Polygon srcPolygon;
                rcv->initPolygon(&rcvPolygon);
                src->initPolygon(&srcPolygon);
                shaft.constructFromPolygonToPolygon(&rcvPolygon, &srcPolygon);
            }
        } else {
            BoundingBox boundingBox;
            patch->computeAndGetBoundingBox(&boundingBox);
            shaft.constructFromBoundingBoxes(topLevelBoundingBox, &boundingBox);
        }

        shaft.setShaftOmit(topElement->patch);
        shaft.setShaftOmit(patch);
        ArrayList<Geometry*> *arr = new ArrayList<Geometry*>();
        shaft.doCulling(oldCandidateList, *candidateList, galerkinState->shaftCullStrategy);
        *candidateList = arr;

        if ( shaft.isCut() ) {
            // One patch causes full occlusion
            Shaft::freeCandidateList(*candidateList);
            *candidateList = oldCandidateList;
            return;
        }
    }

    Interaction link = Interaction();
    const int KSize = GALERKIN_MAX_BASIS_SIZE * GALERKIN_MAX_BASIS_SIZE;
    ensureInteractionCoefficientsPool();
    bool usingPool = true;
    link.K = gInteractionCoefficientsPool.allocate(KSize);
    if ( link.K == NULL ) {
        if ( gInteractionCoefficientsPool.expand(KSize * 128) ) {
            link.K = gInteractionCoefficientsPool.allocate(KSize);
        }
        if ( link.K == NULL ) {
            usingPool = false;
            link.K = new float[KSize];
        }
    }
    link.ownsK = !usingPool;
    link.receiverElement = rcv;
    link.sourceElement = src;

    if ( rcv != NULL ) {
        link.nmbrOBasisFnctnORecv = rcv->basisSize;
    }

    if ( src != NULL ) {
        link.numberOfBasisFunctionsOnSource = src->basisSize;
    }

    bool isSceneGeometry = (*candidateList == scene->geometryList);
    bool isClusteredGeometry = (*candidateList == scene->clusteredGeometryList);
    const ArrayList<Geometry *> *geometryListReferences = *candidateList;
    FormFactorStrategy::cmptAreaTAreaFormFactorVis(
        scene->voxelGrid,
        geometryListReferences,
        isSceneGeometry,
        isClusteredGeometry,
        &link,
        galerkinState);

    if ( galerkinState->exactVisibility || galerkinState->shaftCullMode == ALWAYS_DO_SHAFT_CULLING ) {
        if ( oldCandidateList != *candidateList ) {
            Shaft::freeCandidateList(*candidateList);
        }
        *candidateList = oldCandidateList;
    }

    if ( link.visibility > 0 ) {
        Interaction *newLink = Interaction::interactionDuplicate(&link);
        // Store interactions with the source patch for the progressive radiosity method
        // and with the receiving patch for gathering methods
        if ( galerkinState->galerkinIterationMethod == SOUTH_WELL ) {
            if ( src != NULL ) {
                src->interactions->add(newLink);
            }
        } else if ( rcv != NULL ) {
            rcv->interactions->add(newLink);
        }
    }

    if ( usingPool ) {
        link.K = NULL;
        gInteractionCoefficientsPool.free(KSize);
    }
}

/**
Yes... we exploit the hierarchical structure of the scene during initial linking
*/
void
LinkingSimpleStrategy::geometryLink(
    const Scene *scene,
    const GalerkinState *galerkinState,
    const GalerkinRole role,
    ArrayList<Geometry *> **candidateList,
    GalerkinElement *topElement,
    BoundingBox *topLevelBoundingBox,
    Geometry *geometry)
{
    // Immediately return if the Geometry is bounded and behind the plane of the patch for which interactions are created
    if ( geometry->bounded
        && geometry->getBoundingBox().behindPlane(&topElement->patch->normal, topElement->patch->planeConstant) ) {
        return;
    }

    // If the geometry is bounded, do shaft culling, reducing the candidate list
    // which contains the possible occluder between a pair of patches for which
    // an initial link will need to be created
    ArrayList<Geometry *> *oldCandidateList = *candidateList;

    if ( geometry->bounded && oldCandidateList ) {
        Shaft shaft;
        BoundingBox boundingBox = geometry->getBoundingBox();
        shaft.constructFromBoundingBoxes(topLevelBoundingBox, &boundingBox);
        shaft.setShaftOmit(topElement->patch);
        ArrayList<Geometry*> *arr = new ArrayList<Geometry*>();
        shaft.doCulling(oldCandidateList, arr, galerkinState->shaftCullStrategy);
        *candidateList = arr;
    }

    // If the Geometry is an aggregate, test each of its children GEOMs, if it
    // is a primitive, create an initial link with each patch it consists of
    if ( geometry->isCompound() ) {
        ArrayList<Geometry *> *geometryList = Geometry::primitiveListCopy(geometry);
        for ( int i = 0; geometryList != NULL && i < geometryList->size(); i++ ) {
            LinkingSimpleStrategy::geometryLink(
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
        const ArrayList<Patch *> *patchList = Geometry::patchListReference(geometry);
        for ( int i = 0; patchList != NULL && i < patchList->size(); i++ ) {
            LinkingSimpleStrategy::createInitialLink(
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
        Shaft::freeCandidateList(*candidateList);
    }
    *candidateList = oldCandidateList;
}

/**
Creates the initial interactions for a top level element which is
considered to be a SOURCE or RECEIVER according to 'role'. Interactions
are stored at element for both source and receiver when doing gathering and at the
source element only when doing shooting
*/
void
LinkingSimpleStrategy::createInitialLinks(
    const Scene *scene,
    const GalerkinState *galerkinState,
    const GalerkinRole role,
    GalerkinElement *topElement)
{
    if ( topElement->flags & IS_CLUSTER_MASK ) {
        Error::fatal(-1, "createInitialLinks", "cannot use this routine for cluster elements");
    }

    BoundingBox topLevelBoundingBox;
    topElement->patch->computeAndGetBoundingBox(&topLevelBoundingBox);

    ArrayList<Geometry *> *candidateList = scene->clusteredGeometryList;

    for ( int i = 0; scene->geometryList != NULL && i < scene->geometryList->size(); i++ ) {
        LinkingSimpleStrategy::geometryLink(
            scene,
            galerkinState,
            role,
            &candidateList,
            topElement,
            &topLevelBoundingBox,
            scene->geometryList->get(i));
    }
}
