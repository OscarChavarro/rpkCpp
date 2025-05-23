/**
Southwell Galerkin radiosity (progressive refinement radiosity)
*/

#include "java/util/ArrayList.txx"
#include "common/Statistics.h"
#include "render/potential.h"
#include "render/opengl.h"
#include "GALERKIN/GalerkinRole.h"
#include "GALERKIN/GalerkinRadianceMethod.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/processing/HierarchicalRefinementStrategy.h"
#include "GALERKIN/processing/LinkingClusteredStrategy.h"
#include "GALERKIN/processing/LinkingSimpleStrategy.h"
#include "GALERKIN/processing/ShootingStrategy.h"

/**
Returns the patch with highest un-shot power, weighted with indirect
importance if importance-driven (see Bekaert & Willems, "Importance-driven
Progressive refinement radiosity", EGRW'95, Dublin
*/
Patch *
ShootingStrategy::chooseRadianceShootingPatch(const java::ArrayList<Patch *> *scenePatches, const GalerkinState *galerkinState) {
    Patch *shooting_patch;
    Patch *pot_shooting_patch;
    float power;
    float maximumPower;
    float powerImportance;
    float maximumPowerImportance;

    maximumPower = maximumPowerImportance = 0.0f;
    shooting_patch = pot_shooting_patch = nullptr;
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);

        power = (float)M_PI * patch->area * patch->radianceData->unShotRadiance[0].sumAbsComponents();
        if ( power > maximumPower ) {
            shooting_patch = patch;
            maximumPower = power;
        }

        if ( galerkinState->importanceDriven ) {
            // For importance-driven progressive refinement radiosity, choose the patch
            // with highest indirectly received potential times power
            powerImportance = (galerkinGetPotential(patch) - patch->directPotential) * power;
            if ( powerImportance > maximumPowerImportance ) {
                pot_shooting_patch = patch;
                maximumPowerImportance = powerImportance;
            }
        }
    }

    if ( galerkinState->importanceDriven && pot_shooting_patch ) {
        return pot_shooting_patch;
    }
    return shooting_patch;
}

void
ShootingStrategy::clearUnShotRadianceAndPotential(GalerkinElement *elem) {
    if ( elem->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++) {
            clearUnShotRadianceAndPotential((GalerkinElement *)elem->regularSubElements[i]);
        }
    }

    for ( int i = 0; elem->irregularSubElements != nullptr && i < elem->irregularSubElements->size(); i++ ) {
        clearUnShotRadianceAndPotential((GalerkinElement *)elem->irregularSubElements->get(i));
    }

    colorsArrayClear(elem->unShotRadiance, elem->basisSize);
    elem->unShotPotential = 0.0f;
}

/**
Creates initial links if necessary. Propagates the un-shot radiance of
the patch into the environment. Finally clears the un-shot radiance
at all levels of the element hierarchy for the patch
*/
void
ShootingStrategy::patchPropagateUnShotRadianceAndPotential(
    const Scene *scene,
    const Patch *patch,
    GalerkinState *galerkinState)
{
    GalerkinElement *topLevelElement = galerkinGetElement(patch);

    if ( !(topLevelElement->flags & ElementFlags::INTERACTIONS_CREATED_MASK) ) {
        if ( galerkinState->clustered ) {
            LinkingClusteredStrategy::createInitialLinks(topLevelElement, GalerkinRole::SOURCE, galerkinState);
        } else {
            LinkingSimpleStrategy::createInitialLinks(
                scene,
                galerkinState,
                GalerkinRole::SOURCE,
                topLevelElement);
        }
        topLevelElement->flags |= ElementFlags::INTERACTIONS_CREATED_MASK;
    }

    // Recursively refines the interactions of the shooting patch
    // and computes radiance and potential transport
    HierarchicalRefinementStrategy::refineInteractions(
        scene,
        topLevelElement,
        galerkinState);

    // Clear the un-shot radiance at all levels
    clearUnShotRadianceAndPotential(topLevelElement);
}

/**
Makes the hierarchical representation of potential consistent over all
all levels
*/
float
ShootingStrategy::shootingPushPullPotential(GalerkinElement *element, float down) {
    down += element->receivedPotential / element->area;
    element->receivedPotential = 0.0f;

    float up = 0.0f;

    if ( !element->regularSubElements && !element->irregularSubElements ) {
        up = down;
    }

    if ( element->regularSubElements ) {
        for ( int i = 0; i < 4; i++ ) {
            up += 0.25f * shootingPushPullPotential((GalerkinElement *)element->regularSubElements[i], down);
        }
    }

    if ( element->irregularSubElements ) {
        for ( int j = 0; element->irregularSubElements != nullptr && j < element->irregularSubElements->size(); j++ ) {
            GalerkinElement *subElement = (GalerkinElement *)element->irregularSubElements->get(j);
            if ( !element->isCluster() ) {
                down = 0.0;
            }
            // Don't push to irregular surface sub-elements
            up += subElement->area / element->area * shootingPushPullPotential(subElement, down);
        }
    }

    element->potential += up;
    element->unShotPotential += up;
    return up;
}

void
ShootingStrategy::patchUpdateRadianceAndPotential(const Patch *patch, GalerkinState *galerkinState) {
    GalerkinElement *topLevelElement = galerkinGetElement(patch);
    if ( galerkinState->importanceDriven ) {
        shootingPushPullPotential(topLevelElement, 0.0f);
    }
    basisGalerkinPushPullRadiance(topLevelElement, galerkinState);

    galerkinState->ambientRadiance.addScaled(
        galerkinState->ambientRadiance,
        patch->area,
        patch->radianceData->unShotRadiance[0]);
}

void
ShootingStrategy::doPropagate(const Scene *scene, const Patch *shootingPatch, GalerkinState *galerkinState) {
    // Propagate the un-shot power of the shooting patch into the environment
    patchPropagateUnShotRadianceAndPotential(scene, shootingPatch, galerkinState);

    // Recompute the colors of all patches, not only the patches that received
    // radiance from the shooting patch, since the ambient term has also changed
    if ( galerkinState->clustered ) {
        if ( galerkinState->importanceDriven ) {
            shootingPushPullPotential(galerkinState->topCluster, 0.0);
        }
        basisGalerkinPushPullRadiance(galerkinState->topCluster, galerkinState);
        galerkinState->ambientRadiance = galerkinState->topCluster->unShotRadiance[0];
    } else {
        galerkinState->ambientRadiance.clear();
        for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
            patchUpdateRadianceAndPotential(scene->patchList->get(i), galerkinState);
        }
        galerkinState->ambientRadiance.scale(1.0f / GLOBAL_statistics.totalArea);
    }

    for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
        GalerkinRadianceMethod::recomputePatchColor(scene->patchList->get(i));
    }
}

bool
ShootingStrategy::propagateRadiance(const Scene *scene, GalerkinState *galerkinState) {
    // Choose a shooting patch. also accumulates the total un-shot power into
    // galerkinState->ambientRadiance
    const Patch *shootingPatch = chooseRadianceShootingPatch(scene->patchList, galerkinState);
    if ( !shootingPatch ) {
        return true;
    }

    ColorRgb yellow = {1.0, 1.0, 0.0};
    openGlRenderSetColor(&yellow);
    openGlRenderPatchOutline(shootingPatch);

    doPropagate(
        scene,
        shootingPatch,
        galerkinState);

    return false;
}

/**
Recomputes the potential and un-shot potential of the cluster and its sub-clusters
based on the potential of the contained patches
*/
void
ShootingStrategy::clusterUpdatePotential(GalerkinElement *clusterElement) {
    if ( clusterElement->isCluster() ) {
        clusterElement->potential = 0.0f;
        clusterElement->unShotPotential = 0.0f;
        for ( int i = 0; clusterElement->irregularSubElements != nullptr && i < clusterElement->irregularSubElements->size(); i++ ) {
            GalerkinElement *subCluster = (GalerkinElement *)clusterElement->irregularSubElements->get(i);
            clusterUpdatePotential(subCluster);
            clusterElement->potential += subCluster->area * subCluster->potential;
            clusterElement->unShotPotential += subCluster->area * subCluster->unShotPotential;
        }
        clusterElement->potential /= clusterElement->area;
        clusterElement->unShotPotential /= clusterElement->area;
    }
}

/**
Chooses the patch with highest un-shot importance (potential times
area), see Bekaert & Willems, EGRW'95 (Dublin)
*/
Patch *
ShootingStrategy::choosePotentialShootingPatch(const java::ArrayList<Patch *> *scenePatches) {
    float maximumImportance = 0.0f;
    Patch *shootingPatch = nullptr;

    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        float imp = patch->area * java::Math::abs(galerkinGetUnShotPotential(patch));

        if ( imp > maximumImportance ) {
            shootingPatch = patch;
            maximumImportance = imp;
        }
    }

    return shootingPatch;
}

void
ShootingStrategy::propagatePotential(const Scene *scene, GalerkinState *galerkinState) {
    const Patch *shootingPatch;

    shootingPatch = choosePotentialShootingPatch(scene->patchList);
    if ( shootingPatch ) {
        ColorRgb white = {1.0, 1.0, 1.0};

        openGlRenderSetColor(&white);
        openGlRenderPatchOutline(shootingPatch);
        doPropagate(scene, shootingPatch, galerkinState);
    } else {
        fprintf(stderr, "No patches with un-shot potential??\n");
    }
}

/**
Called for each patch after direct potential has changed (because the
virtual camera has changed)
*/
void
ShootingStrategy::shootingUpdateDirectPotential(GalerkinElement *galerkinElement, float potentialIncrement) {
    if ( galerkinElement->regularSubElements ) {
        for ( int i = 0; i < 4; i++ ) {
            shootingUpdateDirectPotential((GalerkinElement *)galerkinElement->regularSubElements[i], potentialIncrement);
        }
    }
    galerkinElement->directPotential += potentialIncrement;
    galerkinElement->potential += potentialIncrement;
    galerkinElement->unShotPotential += potentialIncrement;
}

/**
One step of the progressive refinement radiosity algorithm

Returns true when converged and false if not
*/
bool
ShootingStrategy::doShootingStep(Scene *scene, GalerkinState *galerkinState, const RenderOptions *renderOptions) {
    if ( galerkinState->importanceDriven ) {
        if ( galerkinState->iterationNumber <= 1 || scene->camera->changed ) {
            updateDirectPotential(scene, renderOptions);
            for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
                const Patch *patch = scene->patchList->get(i);
                GalerkinElement *topLevelElement = galerkinGetElement(patch);
                float potential_increment = patch->directPotential - topLevelElement->directPotential;
                shootingUpdateDirectPotential(topLevelElement, potential_increment);
            }
            scene->camera->changed = false;
            if ( galerkinState->clustered ) {
                clusterUpdatePotential(galerkinState->topCluster);
            }
        }
        propagatePotential(scene, galerkinState);
    }
    return propagateRadiance(scene, galerkinState);
}
