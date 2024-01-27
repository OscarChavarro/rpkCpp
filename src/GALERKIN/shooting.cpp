/**
Southwell Galerkin radiosity (progressive refinement radiosity)
*/

#include <cstdio>

#include "material/statistics.h"
#include "common/error.h"
#include "shared/options.h"
#include "skin/Vertex.h"
#include "shared/Camera.h"
#include "shared/potential.h"
#include "shared/render.h"
#include "GALERKIN/galerkinP.h"
#include "GALERKIN/clustergalerkincpp.h"
#include "GALERKIN/basisgalerkin.h"

/* Returns the patch with highest unshot power, weighted with indirect
 * importance if importance-driven (see Bekaert&Willems, "Importance-driven
 * Progressive refinement radiosity", EGRW'95, Dublin. */
static Patch *
chooseRadianceShootingPatch() {
    Patch *shooting_patch, *pot_shooting_patch;
    float power, maxpower, powerimp, maxpowerimp;
    PatchSet *pl;

    maxpower = maxpowerimp = 0.;
    shooting_patch = pot_shooting_patch = (Patch *) nullptr;
    for ( pl = GLOBAL_scene_patches; pl; pl = pl->next ) {
        Patch *patch = pl->patch;

        power = M_PI * patch->area * colorSumAbsComponents(UNSHOT_RADIANCE(patch));
        if ( power > maxpower ) {
            shooting_patch = patch;
            maxpower = power;
        }

        if ( GLOBAL_galerkin_state.importance_driven ) {
            /* for importance-driven progressive refinement radiosity, choose the patch
             * with highest indirectly received potential times power. */
            powerimp = (POTENTIAL(patch).f - patch->directPotential) * power;
            if ( powerimp > maxpowerimp ) {
                pot_shooting_patch = patch;
                maxpowerimp = powerimp;
            }
        }
    }

    if ( GLOBAL_galerkin_state.importance_driven && pot_shooting_patch ) {
        return pot_shooting_patch;
    }
    return shooting_patch;
}

/* Propagates unshot radiance and potential from source to receiver over the link. */
void
shootUnshotRadianceAndPotentialOverLink(INTERACTION *link) {
    COLOR *srcrad, *rcvrad;

    srcrad = link->sourceElement->unShotRadiance;
    rcvrad = link->receiverElement->receivedRadiance;

    if ( link->nrcv == 1 && link->nsrc == 1 ) {
        colorAddScaled(rcvrad[0], link->K.f, srcrad[0], rcvrad[0]);
    } else {
        int alpha, beta, a, b;
        a = MIN(link->nrcv, link->receiverElement->basisSize);
        b = MIN(link->nsrc, link->sourceElement->basisSize);
        for ( alpha = 0; alpha < a; alpha++ ) {
            for ( beta = 0; beta < b; beta++ ) {
                colorAddScaled(rcvrad[alpha], link->K.p[alpha * link->nsrc + beta], srcrad[beta], rcvrad[alpha]);
            }
        }
    }

    if ( GLOBAL_galerkin_state.importance_driven ) {
        /* propagate unshot potential of the source to the receiver */
        float K = ((link->nrcv == 1 && link->nsrc == 1) ? link->K.f : link->K.p[0]);
        COLOR srcrho;

        if ( isCluster(link->sourceElement)) {
            colorSetMonochrome(srcrho, 1.);
        } else {
            srcrho = REFLECTIVITY(link->sourceElement->patch);
        }
        link->receiverElement->receivedPotential.f += K * colorMaximumComponent(srcrho) * link->sourceElement->unShotPotential.f;
    }
}

static void
clearUnshotRadianceAndPotential(GalerkinElement *elem) {
    ITERATE_REGULAR_SUB_ELEMENTS(elem, clearUnshotRadianceAndPotential);
    ITERATE_IRREGULAR_SUBELEMENTS(elem, clearUnshotRadianceAndPotential);
    clusterGalerkinClearCoefficients(elem->unShotRadiance, elem->basisSize);
    elem->unShotPotential.f = 0.;
}

/* Creates initial links if necessary. Propagates the unshot radiance of
 * the patch into the environment. Finally clears the unshot radiance
 * at all levels of the element hierarchy for the patch. */
static void
patchPropagateUnshotRadianceAndPotential(Patch *shooting_patch) {
    GalerkinElement *top = TOPLEVEL_ELEMENT(shooting_patch);

    if ( !(top->flags & INTERACTIONS_CREATED)) {
        if ( GLOBAL_galerkin_state.clustered ) {
            createInitialLinkWithTopCluster(top, SOURCE);
        } else {
            createInitialLinks(top, SOURCE);
        }
        top->flags |= INTERACTIONS_CREATED;
    }

    /* Recusrively refines the interactions of the shooting patch
     * and computes radiance and potential transport. */
    refineInteractions(top);
    /* EnforceEnergyConservation(shooting_patch); should be done before transport */

    /* Clear the unshot radiance at all levels */
    clearUnshotRadianceAndPotential(top);
}

/* Makes the hierarchical representation of potential consistent over all
 * all levels. */
static float
shootingPushPullPotential(GalerkinElement *elem, float down) {
    float up;
    int i;

    down += elem->receivedPotential.f / elem->area;
    elem->receivedPotential.f = 0.;

    up = 0.;

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
            if ( !isCluster(elem)) {
                down = 0.;
            }
            // Don't push to irregular surface sub-elements
            up += subElement->area / elem->area * shootingPushPullPotential(subElement, down);
        }
    }

    elem->potential.f += up;
    elem->unShotPotential.f += up;
    return up;
}

static void
patchUpdateRadianceAndPotential(Patch *patch) {
    if ( GLOBAL_galerkin_state.importance_driven ) {
        shootingPushPullPotential(TOPLEVEL_ELEMENT(patch), 0.);
    }
    basisGalerkinPushPullRadiance(TOPLEVEL_ELEMENT(patch));

    colorAddScaled(GLOBAL_galerkin_state.ambient_radiance, patch->area, UNSHOT_RADIANCE(patch),
                   GLOBAL_galerkin_state.ambient_radiance);
}

static void
doPropagate(Patch *shooting_patch) {
    // Propagate the un-shot power of the shooting patch into the environment
    patchPropagateUnshotRadianceAndPotential(shooting_patch);

    // Recompute the colors of all patches, not only the patches that received
    // radiance from the shooting patch, since the ambient term has also changed
    if ( GLOBAL_galerkin_state.clustered ) {
        if ( GLOBAL_galerkin_state.importance_driven ) {
            shootingPushPullPotential(GLOBAL_galerkin_state.top_cluster, 0.0);
        }
        basisGalerkinPushPullRadiance(GLOBAL_galerkin_state.top_cluster);
        GLOBAL_galerkin_state.ambient_radiance = GLOBAL_galerkin_state.top_cluster->unShotRadiance[0];
    } else {
        colorClear(GLOBAL_galerkin_state.ambient_radiance);
        for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
            patchUpdateRadianceAndPotential(window->patch);
        }
        colorScale(1.0f / GLOBAL_statistics_totalArea, GLOBAL_galerkin_state.ambient_radiance,
                   GLOBAL_galerkin_state.ambient_radiance);
    }

    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        patchRecomputeColor(window->patch);
    }
}

static int
propagateRadiance() {
    Patch *shooting_patch;

    // Choose a shooting patch. also accumulates the total unshot power into
    // GLOBAL_galerkin_state.ambient_radiance
    shooting_patch = chooseRadianceShootingPatch();
    if ( !shooting_patch ) {
        return true;
    }

    renderSetColor(&GLOBAL_material_yellow);
    renderPatchOutline(shooting_patch);

    doPropagate(shooting_patch);

    return false;
}

/* Called for each patch after direct potential has changed (because the 
 * virtual camera has changed). */
void
shootingUpdateDirectPotential(GalerkinElement *elem, float potential_increment) {
    if ( elem->regularSubElements ) {
        int i;
        for ( i = 0; i < 4; i++ ) {
            shootingUpdateDirectPotential(elem->regularSubElements[i], potential_increment);
        }
    }
    elem->directPotential.f += potential_increment;
    elem->potential.f += potential_increment;
    elem->unShotPotential.f += potential_increment;
}

/* Recomputes the potential and unshot potential of the cluster and its subclusters 
 * based on the potential of the contained patches. */
static void
clusterUpdatePotential(GalerkinElement *clus) {
    if ( isCluster(clus)) {
        ELEMENTLIST *subcluslist;
        clus->potential.f = 0.;
        clus->unShotPotential.f = 0.;
        for ( subcluslist = clus->irregularSubElements; subcluslist; subcluslist = subcluslist->next ) {
            GalerkinElement *subclus = subcluslist->element;
            clusterUpdatePotential(subclus);
            clus->potential.f += subclus->area * subclus->potential.f;
            clus->unShotPotential.f += subclus->area * subclus->unShotPotential.f;
        }
        clus->potential.f /= clus->area;
        clus->unShotPotential.f /= clus->area;
    }
}

/* Chooses the patch with highest unshot importance (potential times 
 * area), see Bekaert&Willems, EGRW'95 (Dublin) */
static Patch *
choosePotentialShootingPatch() {
    float maximp = 0.;
    Patch *shooting_patch = (Patch *) nullptr;
    PatchSet *pl;

    for ( pl = GLOBAL_scene_patches; pl; pl = pl->next ) {
        Patch *patch = pl->patch;
        float imp = patch->area * fabs(UNSHOT_POTENTIAL(patch).f);

        if ( imp > maximp ) {
            shooting_patch = patch;
            maximp = imp;
        }
    }

    return shooting_patch;
}

static void
propagatePotential() {
    Patch *shooting_patch;

    shooting_patch = choosePotentialShootingPatch();
    if ( shooting_patch ) {
        renderSetColor(&GLOBAL_material_white);
        renderPatchOutline(shooting_patch);
        doPropagate(shooting_patch);
    } else {
        fprintf(stderr, "No patches with unshot potential??\n");
    }
}

/**
One step of the progressive refinement radiosity algorithm
*/
int
reallyDoShootingStep() {
    if ( GLOBAL_galerkin_state.importance_driven ) {
        if ( GLOBAL_galerkin_state.iteration_nr <= 1 || GLOBAL_camera_mainCamera.changed ) {
            updateDirectPotential();
            for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
                GalerkinElement *top = TOPLEVEL_ELEMENT(window->patch);
                float potential_increment = window->patch->directPotential - top->directPotential.f;
                shootingUpdateDirectPotential(top, potential_increment);
            }
            GLOBAL_camera_mainCamera.changed = false;
            if ( GLOBAL_galerkin_state.clustered ) {
                clusterUpdatePotential(GLOBAL_galerkin_state.top_cluster);
            }
        }
        propagatePotential();
    }
    return propagateRadiance();
}

/**
Returns true when converged and false if not
*/
int
doShootingStep() {
    return reallyDoShootingStep();
}

void
shootingUpdateMaterial(Material *oldMaterial, Material *newMaterial) {
    logWarning("shootingUpdateMaterial", "Out of order");
}
