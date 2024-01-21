/**
Jocabi or Gauss-Seidel Galerkin radiosity
*/

#include "java/util/ArrayList.txx"
#include "scene/scene.h"
#include "skin/Vertex.h"
#include "shared/Camera.h"
#include "shared/potential.h"
#include "material/statistics.h"
#include "common/error.h"
#include "GALERKIN/interactionlist.h"
#include "GALERKIN/galerkinP.h"

/**
Lazy linking: delay creating the initial links for a patch until it has
radiance to distribute in the environment. Leads to faster feedback to the
user during the first Jacobi iterations. During the first iteration, only
interactions with light sources are created. See Holschuch, EGRW '94
(Darmstadt - EuroGraphics Rendering Workshop).
*/
static void
patchLazyCreateInteractions(Patch *P) {
    GalerkinElement *el = TOPLEVEL_ELEMENT(P);

    if ( !colorNull(el->radiance[0]) && !(el->flags & INTERACTIONS_CREATED)) {
        createInitialLinks(el, SOURCE);
        el->flags |= INTERACTIONS_CREATED;
    }
}

/**
Converts the accumulated received radiance into exitant radiance, making the
hierarchical representation consistent and computes a new color for the patch
*/
static void
patchUpdateRadiance(Patch *P) {
    basisGalerkinPushPullRadiance(TOPLEVEL_ELEMENT(P));
    patchRecomputeColor(P);
}

/**
Creates initial interactions if necessary, recursively refines the interactions
of the toplevel element, gathers radiance over the resulting lowest level
interactions and updates the radiance for the patch if doing
Gauss-Seidel iterations
*/
static void
patchGather(Patch *P) {
    GalerkinElement *el = TOPLEVEL_ELEMENT(P);

    /* don't gather to patches without importance. This optimisation can not
     * be combined with lazy linking based on radiance. */
    if ( GLOBAL_galerkin_state.importance_driven &&
         el->potential.f < GLOBAL_statistics_maxDirectPotential * EPSILON ) {
        return;
    }

    /* the form factors have been computed and stored with the source patch
     * already before when doing non-importance-driven Jacobi iterations with lazy
     * linking. */
    if ( GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL || !GLOBAL_galerkin_state.lazy_linking ||
         GLOBAL_galerkin_state.importance_driven ) {
        if ( !(el->flags & INTERACTIONS_CREATED)) {
            createInitialLinks(el, RECEIVER);
            el->flags |= INTERACTIONS_CREATED;
        }
    }

    /* Refine the interactions and compute light transport at the leaves */
    refineInteractions(el);

    /* Immediately convert received radiance into radiance, make the representation
     * consistent and recompute the color of the patch when doing Gauss-Seidel.
     * The new radiance values are immediately used in subsequent steps of
     * the current iteration. */
    if ( GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ) {
        patchUpdateRadiance(P);
    }
}

/**
Updates the potential of the element after a change of the camera, and as such
a potential change in directly received potential
*/
void
gatheringUpdateDirectPotential(GalerkinElement *elem, float potential_increment) {
    if ( elem->regularSubElements ) {
        int i;
        for ( i = 0; i < 4; i++ ) {
            gatheringUpdateDirectPotential(elem->regularSubElements[i], potential_increment);
        }
    }
    elem->directPotential.f += potential_increment;
    elem->potential.f += potential_increment;
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
            if ( !isCluster(elem)) {
                down = 0.0;
            }    /* don't push to irregular surface subelements */
            up += subel->area / elem->area * gatheringPushPullPotential(subel, down);
        }
    }

    return (elem->potential.f = up);
}

static void
patchUpdatePotential(Patch *patch) {
    gatheringPushPullPotential(TOPLEVEL_ELEMENT(patch), 0.);
}

/**
Recomputes the potential of the cluster and its sub-clusters based on the
potential of the contained patches
*/
static float
gatheringClusterUpdatePotential(GalerkinElement *cluster) {
    if ( cluster->flags & IS_CLUSTER ) {
        ELEMENTLIST *subClusterList;
        cluster->potential.f = 0.0;
        for ( subClusterList = cluster->irregularSubElements; subClusterList; subClusterList = subClusterList->next ) {
            GalerkinElement *subCluster = subClusterList->element;
            cluster->potential.f += subCluster->area * gatheringClusterUpdatePotential(subCluster);
        }
        cluster->potential.f /= cluster->area;
    }
    return cluster->potential.f;
}

/**
Does one step of the radiance computations, returns true if the computations
have converged and false if not
*/
int
randomWalkRadiosityDoGatheringIteration() {
    if ( GLOBAL_galerkin_state.importance_driven ) {
        if ( GLOBAL_galerkin_state.iteration_nr <= 1 || GLOBAL_camera_mainCamera.changed ) {
            updateDirectPotential();
            GLOBAL_camera_mainCamera.changed = false;
        }
    }

    /* non importance-driven Jacobi iterations with lazy linking */
    if ( GLOBAL_galerkin_state.iteration_method != GAUSS_SEIDEL && GLOBAL_galerkin_state.lazy_linking &&
         !GLOBAL_galerkin_state.importance_driven ) {
        for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
            patchLazyCreateInteractions(window->patch);
        }
    }

    /* no visualisation with ambient term for gathering radiosity algorithms */
    colorClear(GLOBAL_galerkin_state.ambient_radiance);

    /* one iteration = gather to all patches */
    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        patchGather(window->patch);
    }

    /* update the radiosity after gathering to all patches with Jacobi, immediately
     * update with Gauss-Seidel so the new radiosity are already used for the
     * still-to-be-processed patches in the same iteration. */
    if ( GLOBAL_galerkin_state.iteration_method == JACOBI ) {
        for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
            patchUpdateRadiance(window->patch);
        }
    }

    if ( GLOBAL_galerkin_state.importance_driven ) {
        for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
            patchUpdatePotential(window->patch);
        }
    }

    return false;    /* never done, until we have a better criterium. */
}

/**
wat als ge clustering aan of afzet tijdens de berekeningen?
*/
int
doClusteredGatheringIteration() {
    static double userErrorThreshold;

    if ( GLOBAL_galerkin_state.importance_driven ) {
        if ( GLOBAL_galerkin_state.iteration_nr <= 1 || GLOBAL_camera_mainCamera.changed ) {
            updateDirectPotential();
            for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
                GalerkinElement *top = TOPLEVEL_ELEMENT(window->patch);
                float potential_increment = window->patch->directPotential - top->directPotential.f;
                gatheringUpdateDirectPotential(top, potential_increment);
            }
            gatheringClusterUpdatePotential(GLOBAL_galerkin_state.top_cluster);
            GLOBAL_camera_mainCamera.changed = false;
        }
    }

    printf("Gal iteration %i\n", GLOBAL_galerkin_state.iteration_nr);

    /* initial linking stage is replaced by the creation of a self-link between
     * the whole scene and itself */
    if ( GLOBAL_galerkin_state.iteration_nr <= 1 ) {
        createInitialLinkWithTopCluster(GLOBAL_galerkin_state.top_cluster, RECEIVER);
    }

    userErrorThreshold = GLOBAL_galerkin_state.rel_link_error_threshold;
    /* refines and computes light transport over the refined links */
    refineInteractions(GLOBAL_galerkin_state.top_cluster);

    GLOBAL_galerkin_state.rel_link_error_threshold = userErrorThreshold;

    /* push received radiance down the hierarchy to the leaf elements, where
     * it is multiplied with the reflectivity and the selfemitted radiance added,
     * and finally pulls back up for a consistent multiresolution representation
     * of radiance over all levels. */
    basisGalerkinPushPullRadiance(GLOBAL_galerkin_state.top_cluster);

    if ( GLOBAL_galerkin_state.importance_driven ) {
        gatheringPushPullPotential(GLOBAL_galerkin_state.top_cluster, 0.);
    }

    /* no visualisation with ambient term for gathering radiosity algorithms */
    colorClear(GLOBAL_galerkin_state.ambient_radiance);

    /* update the display colors of the patches */
    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        patchRecomputeColor(window->patch);
    }

    return false; // Never done
}

void
gatheringUpdateMaterial(Material *oldMaterial, Material *newMaterial) {
    logWarning("gatheringUpdateMaterial", "Out of order");
}
