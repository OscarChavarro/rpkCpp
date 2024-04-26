#include "java/util/ArrayList.txx"
#include "scene/Camera.h"
#include "render/potential.h"
#include "material/statistics.h"
#include "GALERKIN/hierefine.h"
#include "GALERKIN/GalerkinRole.h"
#include "GALERKIN/GalerkinRadianceMethod.h"
#include "GALERKIN/basisgalerkin.h"
#include "LinkingSimpleStrategy.h"
#include "GatheringSimpleStrategy.h"

/**
Lazy linking: delay creating the initial links for a patch until it has
radiance to distribute in the environment. Leads to faster feedback to the
user during the first Jacobi iterations. During the first iteration, only
interactions with light sources are created. See Holschuch, EGRW '94
(Darmstadt - EuroGraphics Rendering Workshop).
*/
void
GatheringSimpleStrategy::patchLazyCreateInteractions(
        VoxelGrid *sceneWorldVoxelGrid,
        Patch *patch,
        GalerkinState *galerkinState,
        java::ArrayList<Geometry *> *sceneGeometries,
        java::ArrayList<Geometry *> *sceneClusteredGeometries) {
    GalerkinElement *topLevelElement = galerkinGetElement(patch);

    if ( !topLevelElement->radiance[0].isBlack() && !(topLevelElement->flags & INTERACTIONS_CREATED_MASK) ) {
        LinkingSimpleStrategy::createInitialLinks(
            sceneWorldVoxelGrid,
            topLevelElement,
            SOURCE,
            galerkinState,
            sceneGeometries,
            sceneClusteredGeometries);
        topLevelElement->flags |= INTERACTIONS_CREATED_MASK;
    }
}

GatheringSimpleStrategy::GatheringSimpleStrategy() {
}

GatheringSimpleStrategy::~GatheringSimpleStrategy() {
}

/**
Converts the accumulated received radiance into exitant radiance, making the
hierarchical representation consistent and computes a new color for the patch
*/
void
GatheringSimpleStrategy::patchUpdateRadiance(Patch *patch, GalerkinState *galerkinState) {
    GalerkinElement *topLevelElement = galerkinGetElement(patch);
    basisGalerkinPushPullRadiance(topLevelElement, galerkinState);
    GalerkinRadianceMethod::recomputePatchColor(patch);
}

/**
Creates initial interactions if necessary, recursively refines the interactions
of the toplevel element, gathers radiance over the resulting lowest level
interactions and updates the radiance for the patch if doing
Gauss-Seidel iterations
*/
void
GatheringSimpleStrategy::patchGather(
    Patch *patch,
    Scene *scene,
    GalerkinState *galerkinState,
    RenderOptions *renderOptions)
{
    GalerkinElement *topLevelElement = galerkinGetElement(patch);

    // Don't gather to patches without importance. This optimisation can not
    // be combined with lazy linking based on radiance
    if ( galerkinState->importanceDriven &&
         topLevelElement->potential < GLOBAL_statistics.maxDirectPotential * EPSILON ) {
        return;
    }

    // The form factors have been computed and stored with the source patch
    // already before when doing non-importance-driven Jacobi iterations with lazy
    // linking
    if ( galerkinState->galerkinIterationMethod == GAUSS_SEIDEL || !galerkinState->lazyLinking ||
         galerkinState->importanceDriven ) {
        if ( !(topLevelElement->flags & INTERACTIONS_CREATED_MASK) ) {
            LinkingSimpleStrategy::createInitialLinks(
                    scene->voxelGrid,
                    topLevelElement,
                    RECEIVER,
                    galerkinState,
                    scene->geometryList,
                    scene->clusteredGeometryList);
            topLevelElement->flags |= INTERACTIONS_CREATED_MASK;
        }
    }

    // Refine the interactions and compute light transport at the leaves
    refineInteractions(scene, topLevelElement, galerkinState, renderOptions);

    // Immediately convert received radiance into radiance, make the representation
    // consistent and recompute the color of the patch when doing Gauss-Seidel.
    // The new radiance values are immediately used in subsequent steps of
    // the current iteration
    if ( galerkinState->galerkinIterationMethod == GAUSS_SEIDEL ) {
        GatheringSimpleStrategy::patchUpdateRadiance(patch, galerkinState);
    }
}

void
GatheringSimpleStrategy::patchUpdatePotential(Patch *patch) {
    GalerkinElement *topLevelElement = galerkinGetElement(patch);
    GatheringStrategy::pushPullPotential(topLevelElement, 0.0f);
}

/**
Does one step of the radiance computations, returns true if the computations have converged and false if not
*/
bool
GatheringSimpleStrategy::doGatheringIteration(Scene *scene, GalerkinState *galerkinState, RenderOptions *renderOptions) {
    if ( galerkinState->importanceDriven ) {
        if ( galerkinState->iterationNumber <= 1 || scene->camera->changed ) {
            updateDirectPotential(scene, renderOptions);
            scene->camera->changed = false;
        }
    }

    // Not importance-driven Jacobi iterations with lazy linking
    if ( galerkinState->galerkinIterationMethod != GAUSS_SEIDEL && galerkinState->lazyLinking &&
         !galerkinState->importanceDriven ) {
        for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
            GatheringSimpleStrategy::patchLazyCreateInteractions(
                scene->voxelGrid,
                scene->patchList->get(i),
                galerkinState,
                scene->geometryList,
                scene->clusteredGeometryList);
        }
    }

    // No visualisation with ambient term for gathering radiosity algorithms
    galerkinState->ambientRadiance.clear();

    // One iteration = gather to all patches
    for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
        GatheringSimpleStrategy::patchGather(scene->patchList->get(i), scene, galerkinState, renderOptions);
    }

    // Update the radiosity after gathering to all patches with Jacobi, immediately
    // update with Gauss-Seidel so the new radiosity are already used for the
    // still-to-be-processed patches in the same iteration
    if ( galerkinState->galerkinIterationMethod == JACOBI ) {
        for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
            GatheringSimpleStrategy::patchUpdateRadiance(scene->patchList->get(i), galerkinState);
        }
    }

    if ( galerkinState->importanceDriven ) {
        for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
            GatheringSimpleStrategy::patchUpdatePotential(scene->patchList->get(i));
        }
    }

    return false; // Never done, until we have a better criteria
}
