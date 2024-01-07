/* gathering.c: Jocabi or Gauss-Seidel Galerkin radiosity */

#include "GALERKIN/galerkinP.h"
#include "scene/scene.h"
#include "skin/Vertex.h"
#include "shared/camera.h"
#include "shared/potential.h"
#include "material/statistics.h"
#include "common/error.h"
#include "GALERKIN/interactionlist.h"

/* Lazy linking: delay creating the initial links for a patch until it has
 * radiance to distribute in the environment. Leads to faster feedback to the
 * user during the first Jacobi iterations. During the first iteration, only 
 * interactions with light sources are created. See Holschuch, EGRW '94 
 * (Darmstadt). */
static void PatchLazyCreateInteractions(PATCH *P) {
    ELEMENT *el = TOPLEVEL_ELEMENT(P);

    if ( !colorNull(el->radiance[0]) && !(el->flags & INTERACTIONS_CREATED)) {
        CreateInitialLinks(el, SOURCE);
        el->flags |= INTERACTIONS_CREATED;
    }
}

/* Converts the accumulated received radiance into exitant radiance, making the
 * hierarchical representation consistent and computes a new color for the patch. */
static void PatchUpdateRadiance(PATCH *P) {
    basisGalerkinPushPullRadiance(TOPLEVEL_ELEMENT(P));
    PatchRecomputeColor(P);
}

/* Creates initial interactions if necessary, recursively refines the interactions
 * of the toplevel element, gathers radiance over the resulting lowest level 
 * interactions and updates the radiance for the patch if doing 
 * Gauss-Seidel iterations. */
static void PatchGather(PATCH *P) {
    ELEMENT *el = TOPLEVEL_ELEMENT(P);

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
            CreateInitialLinks(el, RECEIVER);
            el->flags |= INTERACTIONS_CREATED;
        }
    }

    /* Refine the interactions and compute light transport at the leaves */
    RefineInteractions(el);

    /* Immediately convert received radiance into radiance, make the representation
     * consistent and recompute the color of the patch when doing Gauss-Seidel.
     * The new radiance values are immediately used in subsequent steps of
     * the current iteration. */
    if ( GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ) {
        PatchUpdateRadiance(P);
    }
}

/* Updates the potential of the element after a change of the camera, and as such
 * a potential change in directly received potential. */
void
GatheringUpdateDirectPotential(ELEMENT *elem, float potential_increment) {
    if ( elem->regular_subelements ) {
        int i;
        for ( i = 0; i < 4; i++ ) {
            GatheringUpdateDirectPotential(elem->regular_subelements[i], potential_increment);
        }
    }
    elem->direct_potential.f += potential_increment;
    elem->potential.f += potential_increment;
}

/* Makes the representation of potential consistent after an iteration 
 * (potential is always propagated using Jacobi iterations). */
static float
GatheringPushPullPotential(ELEMENT *elem, float down) {
    float up;

    down += elem->received_potential.f / elem->area;
    elem->received_potential.f = 0.;

    up = 0.;

    if ( !elem->regular_subelements && !elem->irregular_subelements ) {
        up = down + elem->pog.patch->directPotential;
    }

    if ( elem->regular_subelements ) {
        int i;
        for ( i = 0; i < 4; i++ ) {
            up += 0.25 * GatheringPushPullPotential(elem->regular_subelements[i], down);
        }
    }

    if ( elem->irregular_subelements ) {
        ELEMENTLIST *subellist;
        for ( subellist = elem->irregular_subelements; subellist; subellist = subellist->next ) {
            ELEMENT *subel = subellist->element;
            if ( !IsCluster(elem)) {
                down = 0.;
            }    /* don't push to irregular surface subelements */
            up += subel->area / elem->area * GatheringPushPullPotential(subel, down);
        }
    }

    return (elem->potential.f = up);
}

static void
PatchUpdatePotential(PATCH *patch) {
    GatheringPushPullPotential(TOPLEVEL_ELEMENT(patch), 0.);
}

/* Recomputes the potential of the cluster and its subclusters based on the 
 * potential of the contained patches. */
static float
ClusterUpdatePotential(ELEMENT *clus) {
    if ( IsCluster(clus)) {
        ELEMENTLIST *subcluslist;
        clus->potential.f = 0.;
        for ( subcluslist = clus->irregular_subelements; subcluslist; subcluslist = subcluslist->next ) {
            ELEMENT *subclus = subcluslist->element;
            clus->potential.f += subclus->area * ClusterUpdatePotential(subclus);
        }
        clus->potential.f /= clus->area;
    }
    return clus->potential.f;
}

/* Does one step of the radiance computations, returns true if the computations
 * have converged and false if not. */
int
DoGatheringIteration() {
    if ( GLOBAL_galerkin_state.importance_driven ) {
        if ( GLOBAL_galerkin_state.iteration_nr <= 1 || GLOBAL_camera_mainCamera.changed ) {
            UpdateDirectPotential();
            GLOBAL_camera_mainCamera.changed = false;
        }
    }

    /* non importance-driven Jacobi iterations with lazy linking */
    if ( GLOBAL_galerkin_state.iteration_method != GAUSS_SEIDEL && GLOBAL_galerkin_state.lazy_linking &&
         !GLOBAL_galerkin_state.importance_driven ) PatchListIterate(GLOBAL_scene_patches, PatchLazyCreateInteractions);

    /* no visualisation with ambient term for gathering radiosity algorithms */
    colorClear(GLOBAL_galerkin_state.ambient_radiance);

    /* one iteration = gather to all patches */
    PatchListIterate(GLOBAL_scene_patches, PatchGather);

    /* update the radiosity after gathering to all patches with Jacobi, immediately
     * update with Gauss-Seidel so the new radiosity are already used for the
     * still-to-be-processed patches in the same iteration. */
    if ( GLOBAL_galerkin_state.iteration_method == JACOBI ) PatchListIterate(GLOBAL_scene_patches, PatchUpdateRadiance);

    if ( GLOBAL_galerkin_state.importance_driven ) PatchListIterate(GLOBAL_scene_patches, PatchUpdatePotential);

    return false;    /* never done, until we have a better criterium. */
}

/* wat als ge clustering aan of afzet tijdens de berekeningen ??? */
int
DoClusteredGatheringIteration() {
    static double user_error_threshold, current_error_threshold;

    if ( GLOBAL_galerkin_state.importance_driven ) {
        if ( GLOBAL_galerkin_state.iteration_nr <= 1 || GLOBAL_camera_mainCamera.changed ) {
            UpdateDirectPotential();
            ForAllPatches(P, GLOBAL_scene_patches)
                        {
                            ELEMENT *top = TOPLEVEL_ELEMENT(P);
                            float potential_increment = P->directPotential - top->direct_potential.f;
                            GatheringUpdateDirectPotential(top, potential_increment);
                        }
            EndForAll;
            ClusterUpdatePotential(GLOBAL_galerkin_state.top_cluster);
            GLOBAL_camera_mainCamera.changed = false;
        }
    }

    printf("Gal iteration %i\n", GLOBAL_galerkin_state.iteration_nr);

    /* initial linking stage is replaced by the creation of a self-link between
     * the whole scene and itself */
    if ( GLOBAL_galerkin_state.iteration_nr <= 1 ) {
        CreateInitialLinkWithTopCluster(GLOBAL_galerkin_state.top_cluster, RECEIVER);
        current_error_threshold = 0.01;
    } else {
        /* sliding error threshold */
        current_error_threshold /= 1.4;
    }

    user_error_threshold = GLOBAL_galerkin_state.rel_link_error_threshold;
    /*
    if (current_error_threshold < user_error_threshold)
      current_error_threshold = user_error_threshold;
    GLOBAL_galerkin_state.rel_link_error_threshold = current_error_threshold;
    */
    /* refines and computes light transport over the refined links */
    RefineInteractions(GLOBAL_galerkin_state.top_cluster);

    GLOBAL_galerkin_state.rel_link_error_threshold = user_error_threshold;

    /* push received radiance down the hierarchy to the leaf elements, where
     * it is multiplied with the reflectivity and the selfemitted radiance added,
     * and finally pulls back up for a consistent multiresolution representation
     * of radiance over all levels. */
    basisGalerkinPushPullRadiance(GLOBAL_galerkin_state.top_cluster);

    if ( GLOBAL_galerkin_state.importance_driven ) {
        GatheringPushPullPotential(GLOBAL_galerkin_state.top_cluster, 0.);
    }

    /* no visualisation with ambient term for gathering radiosity algorithms */
    colorClear(GLOBAL_galerkin_state.ambient_radiance);

    /* update the display colors of the patches */
    PatchListIterate(GLOBAL_scene_patches, PatchRecomputeColor);

    return false; /* never done */
}

void
GatheringUpdateMaterial(Material *oldmaterial, Material *newmaterial) {
    /*TODO*/
    logWarning("GatheringUpdateMaterial", "Out of order");
}
