/**
Southwell Galerkin radiosity (progressive refinement radiosity)
*/

#include "java/util/ArrayList.txx"
#include "material/statistics.h"
#include "render/potential.h"
#include "shared/render.h"
#include "GALERKIN/clustergalerkincpp.h"
#include "GALERKIN/galerkinP.h"
#include "render/opengl.h"

/**
Returns the patch with highest un-shot power, weighted with indirect
importance if importance-driven (see Bekaert&Willems, "Importance-driven
Progressive refinement radiosity", EGRW'95, Dublin
*/
static Patch *
chooseRadianceShootingPatch(java::ArrayList<Patch *> *scenePatches) {
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

        power = (float)M_PI * patch->area * colorSumAbsComponents(UN_SHOT_RADIANCE(patch));
        if ( power > maximumPower ) {
            shooting_patch = patch;
            maximumPower = power;
        }

        if ( GLOBAL_galerkin_state.importance_driven ) {
            // For importance-driven progressive refinement radiosity, choose the patch
            // with highest indirectly received potential times power
            powerImportance = (POTENTIAL(patch) - patch->directPotential) * power;
            if ( powerImportance > maximumPowerImportance ) {
                pot_shooting_patch = patch;
                maximumPowerImportance = powerImportance;
            }
        }
    }

    if ( GLOBAL_galerkin_state.importance_driven && pot_shooting_patch ) {
        return pot_shooting_patch;
    }
    return shooting_patch;
}

static void
clearUnShotRadianceAndPotential(GalerkinElement *elem) {
    if ( elem->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++) {
            clearUnShotRadianceAndPotential(elem->regularSubElements[i]);
        }
    }

    for ( ELEMENTLIST *window = elem->irregularSubElements; window != nullptr; window = window->next ) {
        clearUnShotRadianceAndPotential(window->element);
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
patchPropagateUnShotRadianceAndPotential(Patch *patch) {
    GalerkinElement *topLevelElement = topLevelGalerkinElement(patch);

    if ( !(topLevelElement->flags & INTERACTIONS_CREATED) ) {
        if ( GLOBAL_galerkin_state.clustered ) {
            createInitialLinkWithTopCluster(topLevelElement, SOURCE);
        } else {
            createInitialLinks(topLevelElement, SOURCE);
        }
        topLevelElement->flags |= INTERACTIONS_CREATED;
    }

    // Recursively refines the interactions of the shooting patch
    // and computes radiance and potential transport
    refineInteractions(topLevelElement);

    // Clear the un-shot radiance at all levels
    clearUnShotRadianceAndPotential(topLevelElement);
}

/**
Makes the hierarchical representation of potential consistent over all
all levels
*/
static float
shootingPushPullPotential(GalerkinElement *elem, float down) {
    float up;
    int i;

    down += elem->receivedPotential / elem->area;
    elem->receivedPotential = 0.0f;

    up = 0.0f;

    if ( !elem->regularSubElements && !elem->irregularSubElements ) {
        up = down;
    }

    if ( elem->regularSubElements ) {
        for ( i = 0; i < 4; i++ ) {
            up += 0.25f * shootingPushPullPotential(elem->regularSubElements[i], down);
        }
    }

    if ( elem->irregularSubElements ) {
        ELEMENTLIST *subElementList;
        for ( subElementList = elem->irregularSubElements; subElementList; subElementList = subElementList->next ) {
            GalerkinElement *subElement = subElementList->element;
            if ( !isCluster(elem) ) {
                down = 0.0;
            }
            // Don't push to irregular surface sub-elements
            up += subElement->area / elem->area * shootingPushPullPotential(subElement, down);
        }
    }

    elem->potential += up;
    elem->unShotPotential += up;
    return up;
}

static void
patchUpdateRadianceAndPotential(Patch *patch) {
    GalerkinElement *topLevelElement = topLevelGalerkinElement(patch);
    if ( GLOBAL_galerkin_state.importance_driven ) {
        shootingPushPullPotential(topLevelElement, 0.0f);
    }
    basisGalerkinPushPullRadiance(topLevelElement);

    colorAddScaled(GLOBAL_galerkin_state.ambient_radiance, patch->area, UN_SHOT_RADIANCE(patch),
                   GLOBAL_galerkin_state.ambient_radiance);
}

static void
doPropagate(Patch *shooting_patch, java::ArrayList<Patch *> *scenePatches) {
    // Propagate the un-shot power of the shooting patch into the environment
    patchPropagateUnShotRadianceAndPotential(shooting_patch);

    // Recompute the colors of all patches, not only the patches that received
    // radiance from the shooting patch, since the ambient term has also changed
    if ( GLOBAL_galerkin_state.clustered ) {
        if ( GLOBAL_galerkin_state.importance_driven ) {
            shootingPushPullPotential(GLOBAL_galerkin_state.topCluster, 0.0);
        }
        basisGalerkinPushPullRadiance(GLOBAL_galerkin_state.topCluster);
        GLOBAL_galerkin_state.ambient_radiance = GLOBAL_galerkin_state.topCluster->unShotRadiance[0];
    } else {
        colorClear(GLOBAL_galerkin_state.ambient_radiance);
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            patchUpdateRadianceAndPotential(scenePatches->get(i));
        }
        colorScale(1.0f / GLOBAL_statistics_totalArea, GLOBAL_galerkin_state.ambient_radiance,
                   GLOBAL_galerkin_state.ambient_radiance);
    }

    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        patchRecomputeColor(scenePatches->get(i));
    }
}

static int
propagateRadiance(java::ArrayList<Patch *> *scenePatches) {
    Patch *shooting_patch;

    // Choose a shooting patch. also accumulates the total un-shot power into
    // GLOBAL_galerkin_state.ambient_radiance
    shooting_patch = chooseRadianceShootingPatch(scenePatches);
    if ( !shooting_patch ) {
        return true;
    }

    openGlRenderSetColor(&GLOBAL_material_yellow);
    openGlRenderPatchOutline(shooting_patch);

    doPropagate(shooting_patch, scenePatches);

    return false;
}

/**
Recomputes the potential and un-shot potential of the cluster and its sub-clusters
based on the potential of the contained patches
*/
static void
clusterUpdatePotential(GalerkinElement *cluster) {
    if ( isCluster(cluster) ) {
        cluster->potential = 0.0f;
        cluster->unShotPotential = 0.0f;
        for ( ELEMENTLIST *window = cluster->irregularSubElements; window; window = window->next ) {
            GalerkinElement *subCluster = window->element;
            clusterUpdatePotential(subCluster);
            cluster->potential += subCluster->area * subCluster->potential;
            cluster->unShotPotential += subCluster->area * subCluster->unShotPotential;
        }
        cluster->potential /= cluster->area;
        cluster->unShotPotential /= cluster->area;
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
        float imp = patch->area * fabs(UN_SHOT_POTENTIAL(patch));

        if ( imp > maximumImportance ) {
            shootingPatch = patch;
            maximumImportance = imp;
        }
    }

    return shootingPatch;
}

static void
propagatePotential(java::ArrayList<Patch *> *scenePatches) {
    Patch *shooting_patch;

    shooting_patch = choosePotentialShootingPatch(scenePatches);
    if ( shooting_patch ) {
        openGlRenderSetColor(&GLOBAL_material_white);
        openGlRenderPatchOutline(shooting_patch);
        doPropagate(shooting_patch, scenePatches);
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
            shootingUpdateDirectPotential(elem->regularSubElements[i], potential_increment);
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
reallyDoShootingStep(java::ArrayList<Patch *> *scenePatches) {
    if ( GLOBAL_galerkin_state.importance_driven ) {
        if ( GLOBAL_galerkin_state.iteration_nr <= 1 || GLOBAL_camera_mainCamera.changed ) {
            updateDirectPotential(scenePatches);
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                Patch *patch = scenePatches->get(i);
                GalerkinElement *topLevelElement = topLevelGalerkinElement(patch);
                float potential_increment = patch->directPotential - topLevelElement->directPotential;
                shootingUpdateDirectPotential(topLevelElement, potential_increment);
            }
            GLOBAL_camera_mainCamera.changed = false;
            if ( GLOBAL_galerkin_state.clustered ) {
                clusterUpdatePotential(GLOBAL_galerkin_state.topCluster);
            }
        }
        propagatePotential(scenePatches);
    }
    return propagateRadiance(scenePatches);
}

/**
Returns true when converged and false if not
*/
int
doShootingStep(java::ArrayList<Patch *> *scenePatches) {
    return reallyDoShootingStep(scenePatches);
}
