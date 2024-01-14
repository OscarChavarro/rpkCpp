/**
Jocabi or Gauss-Seidel Galerkin radiosity
*/

#include "java/util/ArrayList.txx"
#include "scene/scene.h"
#include "skin/Vertex.h"
#include "shared/camera.h"
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
    GalerkingElement *el = TOPLEVEL_ELEMENT(P);

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
    GalerkingElement *el = TOPLEVEL_ELEMENT(P);

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
gatheringUpdateDirectPotential(GalerkingElement *elem, float potential_increment) {
    if ( elem->regular_subelements ) {
        int i;
        for ( i = 0; i < 4; i++ ) {
            gatheringUpdateDirectPotential(elem->regular_subelements[i], potential_increment);
        }
    }
    elem->direct_potential.f += potential_increment;
    elem->potential.f += potential_increment;
}

/**
Makes the representation of potential consistent after an iteration
(potential is always propagated using Jacobi iterations)
*/
static float
gatheringPushPullPotential(GalerkingElement *elem, float down) {
    float up;

    down += elem->received_potential.f / elem->area;
    elem->received_potential.f = 0.0;

    up = 0.0;

    if ( !elem->regular_subelements && !elem->irregular_subelements ) {
        up = down + elem->patch->directPotential;
    }

    if ( elem->regular_subelements ) {
        int i;
        for ( i = 0; i < 4; i++ ) {
            up += 0.25f * gatheringPushPullPotential(elem->regular_subelements[i], down);
        }
    }

    if ( elem->irregular_subelements ) {
        ELEMENTLIST *subellist;
        for ( subellist = elem->irregular_subelements; subellist; subellist = subellist->next ) {
            GalerkingElement *subel = subellist->element;
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
gatheringClusterUpdatePotential(GalerkingElement *cluster) {
    if ( cluster->flags & IS_CLUSTER ) {
        ELEMENTLIST *subClusterList;
        cluster->potential.f = 0.0;
        for ( subClusterList = cluster->irregular_subelements; subClusterList; subClusterList = subClusterList->next ) {
            GalerkingElement *subCluster = subClusterList->element;
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
            UpdateDirectPotential();
            GLOBAL_camera_mainCamera.changed = false;
        }
    }

    /* non importance-driven Jacobi iterations with lazy linking */
    if ( GLOBAL_galerkin_state.iteration_method != GAUSS_SEIDEL && GLOBAL_galerkin_state.lazy_linking &&
         !GLOBAL_galerkin_state.importance_driven ) {
        for ( int i = 0; GLOBAL_scene_patches != nullptr && i < GLOBAL_scene_patches->size(); i++ ) {
            patchLazyCreateInteractions(GLOBAL_scene_patches->get(i));
        }
    }

    /* no visualisation with ambient term for gathering radiosity algorithms */
    colorClear(GLOBAL_galerkin_state.ambient_radiance);

    /* one iteration = gather to all patches */
    for ( int i = 0; GLOBAL_scene_patches != nullptr && i < GLOBAL_scene_patches->size(); i++ ) {
        patchGather(GLOBAL_scene_patches->get(i));
    }

    /* update the radiosity after gathering to all patches with Jacobi, immediately
     * update with Gauss-Seidel so the new radiosity are already used for the
     * still-to-be-processed patches in the same iteration. */
    if ( GLOBAL_galerkin_state.iteration_method == JACOBI ) {
        for ( int i = 0; GLOBAL_scene_patches != nullptr && i < GLOBAL_scene_patches->size(); i++ ) {
            patchUpdateRadiance(GLOBAL_scene_patches->get(i));
        }
    }

    if ( GLOBAL_galerkin_state.importance_driven ) {
        for ( int i = 0; GLOBAL_scene_patches != nullptr && i < GLOBAL_scene_patches->size(); i++ ) {
            patchUpdatePotential(GLOBAL_scene_patches->get(i));
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
            UpdateDirectPotential();
            for ( int i = 0; GLOBAL_scene_patches != nullptr && i < GLOBAL_scene_patches->size(); i++ ) {
                Patch *patch = GLOBAL_scene_patches->get(i);
                GalerkingElement *top = TOPLEVEL_ELEMENT(patch);
                float potential_increment = patch->directPotential - top->direct_potential.f;
                gatheringUpdateDirectPotential(top, potential_increment);
            }
            gatheringClusterUpdatePotential(GLOBAL_galerkin_state.topLevelCluster);
            GLOBAL_camera_mainCamera.changed = false;
        }
    }

    printf("Gal iteration %i\n", GLOBAL_galerkin_state.iteration_nr);

    /* initial linking stage is replaced by the creation of a self-link between
     * the whole scene and itself */
    if ( GLOBAL_galerkin_state.iteration_nr <= 1 ) {
        createInitialLinkWithTopCluster(GLOBAL_galerkin_state.topLevelCluster, RECEIVER);
    }

    userErrorThreshold = GLOBAL_galerkin_state.rel_link_error_threshold;
    /* refines and computes light transport over the refined links */
    refineInteractions(GLOBAL_galerkin_state.topLevelCluster);

    GLOBAL_galerkin_state.rel_link_error_threshold = userErrorThreshold;

    /*
    Push received radiance down the hierarchy to the leaf elements, where
    it is multiplied with the reflectivity and the self-emitted radiance added,
    and finally pulls back up for a consistent multi-resolution representation
    of radiance over all levels
    */
    basisGalerkinPushPullRadiance(GLOBAL_galerkin_state.topLevelCluster);

    if ( GLOBAL_galerkin_state.importance_driven ) {
        gatheringPushPullPotential(GLOBAL_galerkin_state.topLevelCluster, 0.0f);
    }

    // No visualisation with ambient term for gathering radiosity algorithms
    colorClear(GLOBAL_galerkin_state.ambient_radiance);

    /* update the display colors of the patches */
    for ( int i = 0; GLOBAL_scene_patches != nullptr && i < GLOBAL_scene_patches->size(); i++ ) {
        patchRecomputeColor(GLOBAL_scene_patches->get(i));
    }

    return false; // Never done
}

void
gatheringUpdateMaterial(Material *oldMaterial, Material *newMaterial) {
    logWarning("gatheringUpdateMaterial", "Out of order");
}
