#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/render/Potential.h"
#include "vsdk/toolkit/galerkin/GalerkinBasis.h"
#include "vsdk/toolkit/galerkin/GalerkinRadianceMethod.h"
#include "vsdk/toolkit/galerkin/processing/GatheringClusteredStrategy.h"
#include "vsdk/toolkit/galerkin/processing/HierarchicalRefinementStrategy.h"
#include "vsdk/toolkit/galerkin/processing/LinkingClusteredStrategy.h"

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
            GatheringClusteredStrategy::updateClusterDirectPotential(static_cast<GalerkinElement *>(element->regularSubElements[i]), potential_increment);
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
    if ( cluster->flags & ElementFlags::IS_CLUSTER_MASK ) {
        cluster->potential = 0.0;
        for ( int i = 0; cluster->irregularSubElements != nullptr && i < cluster->irregularSubElements->size(); i++ ) {
            GalerkinElement *subCluster = static_cast<GalerkinElement *>(cluster->irregularSubElements->get(i));
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
GatheringClusteredStrategy::doGatheringIteration(const Scene *scene, GalerkinState *galerkinState, RendererConfiguration *renderOptions) {
    if ( galerkinState->importanceDriven &&
        ( galerkinState->iterationNumber <= 1 || scene->camera->changed ) ) {
        Potential::updateDirectPotential(scene, renderOptions);
        for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
            Patch *patch = scene->patchList->get(i);
            GalerkinElement *top = static_cast<GalerkinElement *>(patch->getRadianceData());
            float potentialIncrement = patch->getDirectPotential() - top->directPotential;
            GatheringClusteredStrategy::updateClusterDirectPotential(top, potentialIncrement);
        }
        GatheringClusteredStrategy::updatePotential(galerkinState->topCluster);
        scene->camera->changed = false;
    }

    java::System::out.printf("Galerkin (clustered) iteration %i\n", galerkinState->iterationNumber);

    // Initial linking stage is replaced by the creation of a self-link between
    // the whole scene and itself
    if ( galerkinState->iterationNumber <= 1 ) {
        LinkingClusteredStrategy::createInitialLinks(galerkinState->topCluster, GalerkinRole::RECEIVER, galerkinState);
    }

    double userErrorThreshold = galerkinState->relLinkErrorThreshold;

    // Refines and computes light transport over the refined links
    HierarchicalRefinementStrategy::refineInteractions(scene, galerkinState->topCluster, galerkinState);

    // TODO: This makes galerkinState non const. Check if this can be changed
    galerkinState->relLinkErrorThreshold = static_cast<float>(userErrorThreshold);

    // Push received radiance down the hierarchy to the leaf elements, where
    // it is multiplied with the reflectivity and the self-emitted radiance added,
    // and finally pulls back up for a consistent multi-resolution representation
    // of radiance over all levels
    GalerkinBasis::pushPullRadiance(galerkinState->topCluster, galerkinState);

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
