/**
Jocabi or Gauss-Seidel Galerkin radiosity
*/

#include "java/util/ArrayList.txx"
#include "scene/Camera.h"
#include "render/potential.h"
#include "material/statistics.h"
#include "GALERKIN/hierefine.h"
#include "GALERKIN/initiallinking.h"
#include "GALERKIN/GalerkinRadianceMethod.h"
#include "GALERKIN/basisgalerkin.h"

/**
Makes the representation of potential consistent after an iteration
(potential is always propagated using Jacobi iterations)
*/
float
gatheringPushPullPotential(GalerkinElement *element, float down) {
    down += element->receivedPotential / element->area;
    element->receivedPotential = 0.0f;

    float up = 0.0;

    if ( element->regularSubElements == nullptr && element->irregularSubElements != nullptr ) {
        up = down + element->patch->directPotential;
    }

    if ( element->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++ ) {
            up += 0.25f * gatheringPushPullPotential((GalerkinElement *)element->regularSubElements[i], down);
        }
    }

    if ( element->irregularSubElements != nullptr ) {
        for ( int i = 0; element->irregularSubElements != nullptr && i < element->irregularSubElements->size(); i++ ) {
            GalerkinElement *subElement = (GalerkinElement *)element->irregularSubElements->get(i);
            if ( !element->isCluster() ) {
                // Don't push to irregular surface sub-elements
                down = 0.0;
            }
            up += subElement->area / element->area * gatheringPushPullPotential(subElement, down);
        }
    }

    element->potential = up;
    return element->potential;
}

/**
Lazy linking: delay creating the initial links for a patch until it has
radiance to distribute in the environment. Leads to faster feedback to the
user during the first Jacobi iterations. During the first iteration, only
interactions with light sources are created. See Holschuch, EGRW '94
(Darmstadt - EuroGraphics Rendering Workshop).
*/
static void
patchLazyCreateInteractions(Patch *patch, GalerkinState *galerkinState) {
    GalerkinElement *topLevelElement = galerkinGetElement(patch);

    if ( !topLevelElement->radiance[0].isBlack() && !(topLevelElement->flags & INTERACTIONS_CREATED_MASK) ) {
        createInitialLinks(topLevelElement, SOURCE, galerkinState);
        topLevelElement->flags |= INTERACTIONS_CREATED_MASK;
    }
}

/**
Converts the accumulated received radiance into exitant radiance, making the
hierarchical representation consistent and computes a new color for the patch
*/
static void
patchUpdateRadiance(Patch *patch, GalerkinState *galerkinState) {
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
static void
patchGather(Patch *patch, GalerkinState *galerkinState) {
    GalerkinElement *topLevelElement = galerkinGetElement(patch);

    // Don't gather to patches without importance. This optimisation can not
    // be combined with lazy linking based on radiance. */
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
            createInitialLinks(topLevelElement, RECEIVER, galerkinState);
            topLevelElement->flags |= INTERACTIONS_CREATED_MASK;
        }
    }

    // Refine the interactions and compute light transport at the leaves
    refineInteractions(topLevelElement, galerkinState);

    // Immediately convert received radiance into radiance, make the representation
    // consistent and recompute the color of the patch when doing Gauss-Seidel.
    // The new radiance values are immediately used in subsequent steps of
    // the current iteration
    if ( galerkinState->galerkinIterationMethod == GAUSS_SEIDEL ) {
        patchUpdateRadiance(patch, galerkinState);
    }
}

static void
patchUpdatePotential(Patch *patch) {
    GalerkinElement *topLevelElement = galerkinGetElement(patch);
    gatheringPushPullPotential(topLevelElement, 0.0f);
}

/**
Does one step of the radiance computations, returns true if the computations
have converged and false if not
*/
int
galerkinRadiosityDoGatheringIteration(
    java::ArrayList<Patch *> *scenePatches,
    GalerkinState *galerkinState)
{
    if ( galerkinState->importanceDriven ) {
        if ( galerkinState->iterationNumber <= 1 || GLOBAL_camera_mainCamera.changed ) {
            updateDirectPotential(scenePatches);
            GLOBAL_camera_mainCamera.changed = false;
        }
    }

    // Not importance-driven Jacobi iterations with lazy linking
    if ( galerkinState->galerkinIterationMethod != GAUSS_SEIDEL && galerkinState->lazyLinking &&
         !galerkinState->importanceDriven ) {
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            patchLazyCreateInteractions(scenePatches->get(i), galerkinState);
        }
    }

    // No visualisation with ambient term for gathering radiosity algorithms
    galerkinState->ambientRadiance.clear();

    // One iteration = gather to all patches
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        patchGather(scenePatches->get(i), galerkinState);
    }

    // Update the radiosity after gathering to all patches with Jacobi, immediately
    // update with Gauss-Seidel so the new radiosity are already used for the
    // still-to-be-processed patches in the same iteration
    if ( galerkinState->galerkinIterationMethod == JACOBI ) {
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            patchUpdateRadiance(scenePatches->get(i), galerkinState);
        }
    }

    if ( galerkinState->importanceDriven ) {
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            patchUpdatePotential(scenePatches->get(i));
        }
    }

    return false; // Never done, until we have a better criteria
}
