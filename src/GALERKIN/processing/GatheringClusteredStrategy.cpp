#include "java/util/ArrayList.txx"
#include "scene/Camera.h"
#include "render/potential.h"
#include "GALERKIN/hierefine.h"
#include "GALERKIN/GalerkinRole.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/GalerkinRadianceMethod.h"
#include "LinkingClusteredStrategy.h"
#include "GatheringClusteredStrategy.h"

GatheringClusteredStrategy::GatheringClusteredStrategy() {
}

GatheringClusteredStrategy::~GatheringClusteredStrategy() {
}

/**
Updates the potential of the element after a change of the camera, and as such
a potential change in directly received potential
*/
void
GatheringClusteredStrategy::updateClusterDirectPotential(GalerkinElement *element, float potential_increment) {
    if ( element->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++ ) {
            GatheringClusteredStrategy::updateClusterDirectPotential((GalerkinElement *)element->regularSubElements[i], potential_increment);
        }
    }
    element->directPotential += potential_increment;
    element->potential += potential_increment;
}

/**
Recomputes the potential of the cluster and its sub-clusters based on the
potential of the contained patches
*/
float
GatheringClusteredStrategy::updatePotential(GalerkinElement *cluster) {
    if ( cluster->flags & IS_CLUSTER_MASK ) {
        cluster->potential = 0.0;
        for ( int i = 0; cluster->irregularSubElements != nullptr && i < cluster->irregularSubElements->size(); i++ ) {
            GalerkinElement *subCluster = (GalerkinElement *)cluster->irregularSubElements->get(i);
            cluster->potential += subCluster->area * GatheringClusteredStrategy::updatePotential(subCluster);
        }
        cluster->potential /= cluster->area;
    }
    return cluster->potential;
}

/**
Note: clustering should not be turned off during the calculations
*/
bool
GatheringClusteredStrategy::doGatheringIteration(Scene *scene, GalerkinState *galerkinState, RenderOptions *renderOptions) {
    if ( galerkinState->importanceDriven ) {
        if ( galerkinState->iterationNumber <= 1 || scene->camera->changed ) {
            updateDirectPotential(scene, renderOptions);
            for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
                Patch *patch = scene->patchList->get(i);
                GalerkinElement *top = (GalerkinElement *)patch->radianceData;
                float potentialIncrement = patch->directPotential - top->directPotential;
                GatheringClusteredStrategy::updateClusterDirectPotential(top, potentialIncrement);
            }
            GatheringClusteredStrategy::updatePotential(galerkinState->topCluster);
            scene->camera->changed = false;
        }
    }

    printf("Galerkin (clustered) iteration %i\n", galerkinState->iterationNumber);

    // Initial linking stage is replaced by the creation of a self-link between
    // the whole scene and itself
    if ( galerkinState->iterationNumber <= 1 ) {
        LinkingClusteredStrategy::createInitialLinks(galerkinState->topCluster, RECEIVER, galerkinState);
    }

    double userErrorThreshold = galerkinState->relLinkErrorThreshold;

    // Refines and computes light transport over the refined links
    refineInteractions(scene, galerkinState->topCluster, galerkinState, renderOptions);

    // TODO: This makes galerkinState non const. Check if this can be changed
    galerkinState->relLinkErrorThreshold = (float)userErrorThreshold;

    // Push received radiance down the hierarchy to the leaf elements, where
    // it is multiplied with the reflectivity and the self-emitted radiance added,
    // and finally pulls back up for a consistent multi-resolution representation
    // of radiance over all levels
    basisGalerkinPushPullRadiance(galerkinState->topCluster, galerkinState);

    if ( galerkinState->importanceDriven ) {
        GatheringStrategy::pushPullPotential(galerkinState->topCluster, 0.0);
    }

    // No visualisation with ambient term for gathering radiosity algorithms
    galerkinState->ambientRadiance.clear();

    // Update the display colors of the patches
    for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
        GalerkinRadianceMethod::recomputePatchColor(scene->patchList->get(i));
    }

    return false; // Never done
}
