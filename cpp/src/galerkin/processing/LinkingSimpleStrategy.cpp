#include "java/util/ArrayList.txx"
#include "common/Error.h"
#include "skin/Compound.h"
#include "galerkin/processing/FormFactorStrategy.h"
#include "galerkin/processing/LinkingSimpleStrategy.h"
#include "galerkin/Shaft.h"

void
LinkingSimpleStrategy::createInitialLink(
    const Scene *scene,
    const GalerkinState *galerkinState,
    const GalerkinRole role,
    java::ArrayList<PatchSet *> **candidateList,
    GalerkinElement *topElement,
    BoundingBox *topLevelBoundingBox,
    Patch *patch)
{
    if ( !patch->facing(topElement->patch) ) {
        return;
    }

    GalerkinElement *rcv;
    GalerkinElement *src;
    GalerkinElement *topLevelElement = GalerkinElement::fromPatch(patch);
    switch ( role ) {
        case GalerkinRole::SOURCE:
            rcv = topLevelElement;
            src = topElement;
            break;
        case GalerkinRole::RECEIVER:
            rcv = topElement;
            src = topLevelElement;
            break;
        default:
            Error::fatal(2, "createInitialLink", "Impossible element role");
    }

    java::ArrayList<PatchSet *> *oldCandidateList = *candidateList;

    if ( (galerkinState->exactVisibility
       || galerkinState->shaftCullMode == GalerkinShaftCullMode::ALWAYS_DO_SHAFT_CULLING)
       && oldCandidateList ) {
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
            patch->computeAndGetBoundingBox(&boundingBox);
            shaft.constructFromBoundingBoxes(topLevelBoundingBox, &boundingBox);
        }

        shaft.setShaftOmit(topElement->patch);
        shaft.setShaftOmit(patch);
        java::ArrayList<PatchSet*> *arr = new java::ArrayList<PatchSet*>();
        shaft.doCulling(oldCandidateList, arr, galerkinState->shaftCullStrategy);
        *candidateList = arr;

        if ( shaft.isCut() ) {
            // One patch causes full occlusion
            Shaft::freeCandidateList(*candidateList);
            *candidateList = oldCandidateList;
            return;
        }
    }

    Interaction link{};
    link.K = new float[GalerkinBasis::MAX_BASIS_SIZE * GalerkinBasis::MAX_BASIS_SIZE];
    link.receiverElement = rcv;
    link.sourceElement = src;

    if ( rcv != nullptr ) {
        link.numberOfBasisFunctionsOnReceiver = rcv->basisSize;
    }

    if ( src != nullptr ) {
        link.numberOfBasisFunctionsOnSource = src->basisSize;
    }

    bool isSceneGeometry = (*candidateList == galerkinState->scenePatchSetList);
    bool isClusteredGeometry = (*candidateList == galerkinState->clusteredPatchSetList);
    const java::ArrayList<PatchSet *> *geometryListReferences = *candidateList;
    FormFactorStrategy::computeAreaToAreaFormFactorVisibility(
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
void
LinkingSimpleStrategy::geometryLink(
    const Scene *scene,
    const GalerkinState *galerkinState,
    const GalerkinRole role,
    java::ArrayList<PatchSet *> **candidateList,
    GalerkinElement *topElement,
    BoundingBox *topLevelBoundingBox,
    Geometry *geometry)
{
    // Immediately return if the geometry is bounded and behind the plane of the patch for which interactions are created
    if ( geometry->bounded
        && geometry->getBoundingBox().behindPlane(&topElement->patch->getNormal(), topElement->patch->getPlaneConstant()) ) {
        return;
    }

    // If the geometry is bounded, do shaft culling, reducing the candidate list
    // which contains the possible occluder between a pair of patches for which
    // an initial link will need to be created
    java::ArrayList<PatchSet *> *oldCandidateList = *candidateList;

    if ( geometry->bounded && oldCandidateList ) {
        Shaft shaft;
        BoundingBox boundingBox = geometry->getBoundingBox();
        shaft.constructFromBoundingBoxes(topLevelBoundingBox, &boundingBox);
        shaft.setShaftOmit(topElement->patch);
        java::ArrayList<PatchSet*> *arr = new java::ArrayList<PatchSet*>();
        shaft.doCulling(oldCandidateList, arr, galerkinState->shaftCullStrategy);
        *candidateList = arr;
    }

    if ( geometry->isCompound() ) {
        const Compound *compound = static_cast<const Compound *>(geometry);
        for ( int i = 0; compound->children != nullptr && i < compound->children->size(); i++ ) {
            LinkingSimpleStrategy::geometryLink(
                scene,
                galerkinState,
                role,
                candidateList,
                topElement,
                topLevelBoundingBox,
                compound->children->get(i));
        }
    } else {
        const java::ArrayList<Patch *> *patchList = Geometry::patchListReference(geometry);
        for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
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
    if ( topElement->flags & ElementFlags::IS_CLUSTER_MASK ) {
        Error::fatal(-1, "createInitialLinks", "cannot use this routine for cluster elements");
    }

    BoundingBox topLevelBoundingBox;
    topElement->patch->computeAndGetBoundingBox(&topLevelBoundingBox);

    java::ArrayList<PatchSet *> *candidateList = galerkinState->clusteredPatchSetList;

    for ( int i = 0; scene->geometryList != nullptr && i < scene->geometryList->size(); i++ ) {
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
