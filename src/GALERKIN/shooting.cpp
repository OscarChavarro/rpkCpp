/**
Southwell Galerkin radiosity (progressive refinement radiosity)
*/

#include "java/util/ArrayList.txx"
#include "material/statistics.h"
#include "render/potential.h"
#include "render/opengl.h"
#include "render/ScreenBuffer.h"
#include "GALERKIN/clustergalerkincpp.h"
#include "GALERKIN/hierefine.h"
#include "GALERKIN/initiallinking.h"
#include "GALERKIN/GalerkinRadianceMethod.h"
#include "GALERKIN/shooting.h"
#include "GALERKIN/basisgalerkin.h"

static inline float
galerkinGetPotential(Patch *patch) {
    return ((GalerkinElement *)((patch)->radianceData))->potential;
}

static inline float
galerkinGetUnShotPotential(Patch *patch) {
    return ((GalerkinElement *)((patch)->radianceData))->unShotPotential;
}

/**
Returns the patch with highest un-shot power, weighted with indirect
importance if importance-driven (see Bekaert & Willems, "Importance-driven
Progressive refinement radiosity", EGRW'95, Dublin
*/
static Patch *
chooseRadianceShootingPatch(java::ArrayList<Patch *> *scenePatches, GalerkinState *galerkinState) {
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

static void
clearUnShotRadianceAndPotential(GalerkinElement *elem) {
    if ( elem->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++) {
            clearUnShotRadianceAndPotential((GalerkinElement *)elem->regularSubElements[i]);
        }
    }

    for ( int i = 0; elem->irregularSubElements != nullptr && i < elem->irregularSubElements->size(); i++ ) {
        clearUnShotRadianceAndPotential((GalerkinElement *)elem->irregularSubElements->get(i));
    }

    clusterGalerkinClearCoefficients(elem->unShotRadiance, elem->basisSize);
    elem->unShotPotential = 0.0f;
}

/**
Creates initial links if necessary. Propagates the un-shot radiance of
the patch into the environment. Finally clears the un-shot radiance
at all levels of the element hierarchy for the patch
*/
static void
patchPropagateUnShotRadianceAndPotential(Patch *patch, GalerkinState *galerkinState) {
    GalerkinElement *topLevelElement = galerkinGetElement(patch);

    if ( !(topLevelElement->flags & INTERACTIONS_CREATED_MASK) ) {
        if ( galerkinState->clustered ) {
            createInitialLinkWithTopCluster(topLevelElement, SOURCE, galerkinState);
        } else {
            createInitialLinks(topLevelElement, SOURCE, galerkinState);
        }
        topLevelElement->flags |= INTERACTIONS_CREATED_MASK;
    }

    // Recursively refines the interactions of the shooting patch
    // and computes radiance and potential transport
    refineInteractions(topLevelElement, galerkinState);

    // Clear the un-shot radiance at all levels
    clearUnShotRadianceAndPotential(topLevelElement);
}

/**
Makes the hierarchical representation of potential consistent over all
all levels
*/
static float
shootingPushPullPotential(GalerkinElement *element, float down) {
    float up;
    int i;

    down += element->receivedPotential / element->area;
    element->receivedPotential = 0.0f;

    up = 0.0f;

    if ( !element->regularSubElements && !element->irregularSubElements ) {
        up = down;
    }

    if ( element->regularSubElements ) {
        for ( i = 0; i < 4; i++ ) {
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

static void
patchUpdateRadianceAndPotential(Patch *patch, GalerkinState *galerkinState) {
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

static void
doPropagate(
    Patch *shooting_patch,
    java::ArrayList<Patch *> *scenePatches,
    GalerkinState *galerkinState,
    GalerkinRadianceMethod *galerkinRadianceMethod)
{
    // Propagate the un-shot power of the shooting patch into the environment
    patchPropagateUnShotRadianceAndPotential(shooting_patch, galerkinState);

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
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            patchUpdateRadianceAndPotential(scenePatches->get(i), galerkinState);
        }
        galerkinState->ambientRadiance.scale(1.0f / GLOBAL_statistics.totalArea);
    }

    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        galerkinRadianceMethod->recomputePatchColor(scenePatches->get(i));
    }
}

static int
propagateRadiance(
    java::ArrayList<Patch *> *scenePatches,
    GalerkinState *galerkinState,
    GalerkinRadianceMethod *galerkinRadianceMethod)
{
    Patch *shooting_patch;

    // Choose a shooting patch. also accumulates the total un-shot power into
    // galerkinState->ambientRadiance
    shooting_patch = chooseRadianceShootingPatch(scenePatches, galerkinState);
    if ( !shooting_patch ) {
        return true;
    }

    openGlRenderSetColor(&GLOBAL_material_yellow);
    openGlRenderPatchOutline(shooting_patch);

    doPropagate(shooting_patch, scenePatches, galerkinState, galerkinRadianceMethod);

    return false;
}

/**
Recomputes the potential and un-shot potential of the cluster and its sub-clusters
based on the potential of the contained patches
*/
static void
clusterUpdatePotential(GalerkinElement *clusterElement) {
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
static Patch *
choosePotentialShootingPatch(java::ArrayList<Patch *> *scenePatches) {
    float maximumImportance = 0.0f;
    Patch *shootingPatch = nullptr;

    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        float imp = patch->area * std::fabs(galerkinGetUnShotPotential(patch));

        if ( imp > maximumImportance ) {
            shootingPatch = patch;
            maximumImportance = imp;
        }
    }

    return shootingPatch;
}

static void
propagatePotential(
    java::ArrayList<Patch *> *scenePatches,
    GalerkinState *galerkinState,
    GalerkinRadianceMethod *galerkinRadianceMethod)
{
    Patch *shooting_patch;

    shooting_patch = choosePotentialShootingPatch(scenePatches);
    if ( shooting_patch ) {
        openGlRenderSetColor(&GLOBAL_material_white);
        openGlRenderPatchOutline(shooting_patch);
        doPropagate(shooting_patch, scenePatches, galerkinState, galerkinRadianceMethod);
    } else {
        fprintf(stderr, "No patches with un-shot potential??\n");
    }
}

/**
Called for each patch after direct potential has changed (because the
virtual camera has changed)
*/
static void
shootingUpdateDirectPotential(GalerkinElement *elem, float potential_increment) {
    if ( elem->regularSubElements ) {
        int i;
        for ( i = 0; i < 4; i++ ) {
            shootingUpdateDirectPotential((GalerkinElement *)elem->regularSubElements[i], potential_increment);
        }
    }
    elem->directPotential += potential_increment;
    elem->potential += potential_increment;
    elem->unShotPotential += potential_increment;
}

/**
One step of the progressive refinement radiosity algorithm
*/
static int
reallyDoShootingStep(
    java::ArrayList<Patch *> *scenePatches,
    GalerkinState *galerkinState,
    GalerkinRadianceMethod *galerkinRadianceMethod)
{
    if ( galerkinState->importanceDriven ) {
        if ( galerkinState->iterationNumber <= 1 || GLOBAL_camera_mainCamera.changed ) {
            updateDirectPotential(scenePatches);
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                Patch *patch = scenePatches->get(i);
                GalerkinElement *topLevelElement = galerkinGetElement(patch);
                float potential_increment = patch->directPotential - topLevelElement->directPotential;
                shootingUpdateDirectPotential(topLevelElement, potential_increment);
            }
            GLOBAL_camera_mainCamera.changed = false;
            if ( galerkinState->clustered ) {
                clusterUpdatePotential(galerkinState->topCluster);
            }
        }
        propagatePotential(scenePatches, galerkinState, galerkinRadianceMethod);
    }
    return propagateRadiance(scenePatches, galerkinState, galerkinRadianceMethod);
}

/**
Returns true when converged and false if not
*/
int
doShootingStep(
    java::ArrayList<Patch *> *scenePatches,
    GalerkinState *galerkinState,
    GalerkinRadianceMethod *galerkinRadianceMethod)
{
    return reallyDoShootingStep(scenePatches, galerkinState, galerkinRadianceMethod);
}
