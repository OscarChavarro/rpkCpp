/**
Jocabi or Gauss-Seidel Galerkin radiosity
*/

#include "java/util/ArrayList.txx"
#include "shared/Camera.h"
#include "render/potential.h"
#include "material/statistics.h"
#include "GALERKIN/galerkinP.h"

/**
Lazy linking: delay creating the initial links for a patch until it has
radiance to distribute in the environment. Leads to faster feedback to the
user during the first Jacobi iterations. During the first iteration, only
interactions with light sources are created. See Holschuch, EGRW '94
(Darmstadt - EuroGraphics Rendering Workshop).
*/
static void
patchLazyCreateInteractions(Patch *patch) {
    GalerkinElement *topLevelElement = topLevelGalerkinElement(patch);

    if ( !colorNull(topLevelElement->radiance[0]) && !(topLevelElement->flags & INTERACTIONS_CREATED)) {
        createInitialLinks(topLevelElement, SOURCE);
        topLevelElement->flags |= INTERACTIONS_CREATED;
    }
}

/**
Converts the accumulated received radiance into exitant radiance, making the
hierarchical representation consistent and computes a new color for the patch
*/
static void
patchUpdateRadiance(Patch *patch) {
    GalerkinElement *topLevelElement = topLevelGalerkinElement(patch);
    basisGalerkinPushPullRadiance(topLevelElement);
    patchRecomputeColor(patch);
}

/**
Creates initial interactions if necessary, recursively refines the interactions
of the toplevel element, gathers radiance over the resulting lowest level
interactions and updates the radiance for the patch if doing
Gauss-Seidel iterations
*/
static void
patchGather(Patch *patch) {
    GalerkinElement *topLevelElement = topLevelGalerkinElement(patch);

    /* don't gather to patches without importance. This optimisation can not
     * be combined with lazy linking based on radiance. */
    if ( GLOBAL_galerkin_state.importance_driven &&
         topLevelElement->potential < GLOBAL_statistics_maxDirectPotential * EPSILON ) {
        return;
    }

    /* the form factors have been computed and stored with the source patch
     * already before when doing non-importance-driven Jacobi iterations with lazy
     * linking. */
    if ( GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL || !GLOBAL_galerkin_state.lazy_linking ||
         GLOBAL_galerkin_state.importance_driven ) {
        if ( !(topLevelElement->flags & INTERACTIONS_CREATED)) {
            createInitialLinks(topLevelElement, RECEIVER);
            topLevelElement->flags |= INTERACTIONS_CREATED;
        }
    }

    /* Refine the interactions and compute light transport at the leaves */
    refineInteractions(topLevelElement);

    /* Immediately convert received radiance into radiance, make the representation
     * consistent and recompute the color of the patch when doing Gauss-Seidel.
     * The new radiance values are immediately used in subsequent steps of
     * the current iteration. */
    if ( GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ) {
        patchUpdateRadiance(patch);
    }
}

/**
Updates the potential of the element after a change of the camera, and as such
a potential change in directly received potential
*/
static void
gatheringUpdateDirectPotential(GalerkinElement *elem, float potential_increment) {
    if ( elem->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++ ) {
            gatheringUpdateDirectPotential(elem->regularSubElements[i], potential_increment);
        }
    }
    elem->directPotential.f += potential_increment;
    elem->potential += potential_increment;
}

/**
Makes the representation of potential consistent after an iteration
(potential is always propagated using Jacobi iterations)
*/
static float
gatheringPushPullPotential(GalerkinElement *elem, float down) {
    float up;

    down += elem->receivedPotential.f / elem->area;
    elem->receivedPotential.f = 0.0;

    up = 0.0;

    if ( !elem->regularSubElements && !elem->irregularSubElements ) {
        up = down + elem->patch->directPotential;
    }

    if ( elem->regularSubElements ) {
        int i;
        for ( i = 0; i < 4; i++ ) {
            up += 0.25f * gatheringPushPullPotential(elem->regularSubElements[i], down);
        }
    }

    if ( elem->irregularSubElements ) {
        ELEMENTLIST *subellist;
        for ( subellist = elem->irregularSubElements; subellist; subellist = subellist->next ) {
            GalerkinElement *subel = subellist->element;
            if ( !isCluster(elem) ) {
                // Don't push to irregular surface sub-elements
                down = 0.0;
            }
            up += subel->area / elem->area * gatheringPushPullPotential(subel, down);
        }
    }

    return (elem->potential = up);
}

static void
patchUpdatePotential(Patch *patch) {
    GalerkinElement *topLevelElement = topLevelGalerkinElement(patch);
    gatheringPushPullPotential(topLevelElement, 0.0f);
}

/**
Recomputes the potential of the cluster and its sub-clusters based on the
potential of the contained patches
*/
static float
gatheringClusterUpdatePotential(GalerkinElement *cluster) {
    if ( cluster->flags & IS_CLUSTER ) {
        ELEMENTLIST *subClusterList;
        cluster->potential = 0.0;
        for ( subClusterList = cluster->irregularSubElements; subClusterList; subClusterList = subClusterList->next ) {
            GalerkinElement *subCluster = subClusterList->element;
            cluster->potential += subCluster->area * gatheringClusterUpdatePotential(subCluster);
        }
        cluster->potential /= cluster->area;
    }
    return cluster->potential;
}

/**
Does one step of the radiance computations, returns true if the computations
have converged and false if not
*/
int
randomWalkRadiosityDoGatheringIteration(java::ArrayList<Patch *> *scenePatches) {
    if ( GLOBAL_galerkin_state.importance_driven ) {
        if ( GLOBAL_galerkin_state.iteration_nr <= 1 || GLOBAL_camera_mainCamera.changed ) {
            updateDirectPotential(scenePatches);
            GLOBAL_camera_mainCamera.changed = false;
        }
    }

    // Not importance-driven Jacobi iterations with lazy linking
    if ( GLOBAL_galerkin_state.iteration_method != GAUSS_SEIDEL && GLOBAL_galerkin_state.lazy_linking &&
         !GLOBAL_galerkin_state.importance_driven ) {
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            patchLazyCreateInteractions(scenePatches->get(i));
        }
    }

    // No visualisation with ambient term for gathering radiosity algorithms
    colorClear(GLOBAL_galerkin_state.ambient_radiance);

    // One iteration = gather to all patches
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        patchGather(scenePatches->get(i));
    }

    // Update the radiosity after gathering to all patches with Jacobi, immediately
    // update with Gauss-Seidel so the new radiosity are already used for the
    // still-to-be-processed patches in the same iteration
    if ( GLOBAL_galerkin_state.iteration_method == JACOBI ) {
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            patchUpdateRadiance(scenePatches->get(i));
        }
    }

    if ( GLOBAL_galerkin_state.importance_driven ) {
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            patchUpdatePotential(scenePatches->get(i));
        }
    }

    return false; // Never done, until we have a better criteria
}

/**
what if you turn clustering on or off during the calculations?
*/
int
doClusteredGatheringIteration(java::ArrayList<Patch*> *scenePatches) {
    static double userErrorThreshold;

    if ( GLOBAL_galerkin_state.importance_driven ) {
        if ( GLOBAL_galerkin_state.iteration_nr <= 1 || GLOBAL_camera_mainCamera.changed ) {
            updateDirectPotential(scenePatches);
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                Patch *patch = scenePatches->get(i);
                GalerkinElement *top = (GalerkinElement *)patch->radianceData;
                float potential_increment = patch->directPotential - top->directPotential.f;
                gatheringUpdateDirectPotential(top, potential_increment);
            }
            gatheringClusterUpdatePotential(GLOBAL_galerkin_state.topCluster);
            GLOBAL_camera_mainCamera.changed = false;
        }
    }

    printf("Gal iteration %i\n", GLOBAL_galerkin_state.iteration_nr);

    // Initial linking stage is replaced by the creation of a self-link between
    // the whole scene and itself
    if ( GLOBAL_galerkin_state.iteration_nr <= 1 ) {
        createInitialLinkWithTopCluster(GLOBAL_galerkin_state.topCluster, RECEIVER);
    }

    userErrorThreshold = GLOBAL_galerkin_state.relLinkErrorThreshold;

    // Refines and computes light transport over the refined links
    refineInteractions(GLOBAL_galerkin_state.topCluster);

    GLOBAL_galerkin_state.relLinkErrorThreshold = (float)userErrorThreshold;

    // Push received radiance down the hierarchy to the leaf elements, where
    // it is multiplied with the reflectivity and the self-emitted radiance added,
    // and finally pulls back up for a consistent multi-resolution representation
    // of radiance over all levels
    basisGalerkinPushPullRadiance(GLOBAL_galerkin_state.topCluster);

    if ( GLOBAL_galerkin_state.importance_driven ) {
        gatheringPushPullPotential(GLOBAL_galerkin_state.topCluster, 0.);
    }

    // No visualisation with ambient term for gathering radiosity algorithms
    colorClear(GLOBAL_galerkin_state.ambient_radiance);

    // Update the display colors of the patches
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        patchRecomputeColor(scenePatches->get(i));
    }

    return false; // Never done
}
