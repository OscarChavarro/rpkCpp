#include "java/util/ArrayList.txx"
#include "scene/Camera.h"
#include "render/potential.h"
#include "GALERKIN/hierefine.h"
#include "GALERKIN/initiallinking.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/gathering.h"

/**
Updates the potential of the element after a change of the camera, and as such
a potential change in directly received potential
*/
static void
gatheringUpdateDirectPotential(GalerkinElement *elem, float potential_increment) {
    if ( elem->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++ ) {
            gatheringUpdateDirectPotential((GalerkinElement *)elem->regularSubElements[i], potential_increment);
        }
    }
    elem->directPotential += potential_increment;
    elem->potential += potential_increment;
}

/**
Recomputes the potential of the cluster and its sub-clusters based on the
potential of the contained patches
*/
static float
gatheringClusterUpdatePotential(GalerkinElement *cluster) {
    if ( cluster->flags & IS_CLUSTER_MASK ) {
        cluster->potential = 0.0;
        for ( int i = 0; cluster->irregularSubElements != nullptr && i < cluster->irregularSubElements->size(); i++ ) {
            GalerkinElement *subCluster = (GalerkinElement *)cluster->irregularSubElements->get(i);
            cluster->potential += subCluster->area * gatheringClusterUpdatePotential(subCluster);
        }
        cluster->potential /= cluster->area;
    }
    return cluster->potential;
}

/**
what if you turn clustering on or off during the calculations?
*/
int
doClusteredGatheringIteration(
    Camera *camera,
    VoxelGrid *sceneWorldVoxelGrid,
    java::ArrayList<Patch*> *scenePatches,
    java::ArrayList<Geometry *> *sceneGeometries,
    java::ArrayList<Geometry *> *sceneClusteredGeometries,
    Geometry *clusteredWorldGeometry,
    GalerkinState *galerkinState)
{
    if ( galerkinState->importanceDriven ) {
        if ( galerkinState->iterationNumber <= 1 || camera->changed ) {
            updateDirectPotential(scenePatches, clusteredWorldGeometry);
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                Patch *patch = scenePatches->get(i);
                GalerkinElement *top = (GalerkinElement *)patch->radianceData;
                float potentialIncrement = patch->directPotential - top->directPotential;
                gatheringUpdateDirectPotential(top, potentialIncrement);
            }
            gatheringClusterUpdatePotential(galerkinState->topCluster);
            camera->changed = false;
        }
    }

    printf("Gal iteration %i\n", galerkinState->iterationNumber);

    // Initial linking stage is replaced by the creation of a self-link between
    // the whole scene and itself
    if ( galerkinState->iterationNumber <= 1 ) {
        createInitialLinkWithTopCluster(galerkinState->topCluster, RECEIVER, galerkinState);
    }

    double userErrorThreshold = galerkinState->relLinkErrorThreshold;

    // Refines and computes light transport over the refined links
    refineInteractions(
        camera,
        sceneWorldVoxelGrid,
        galerkinState->topCluster,
        galerkinState,
        sceneGeometries,
        sceneClusteredGeometries,
        clusteredWorldGeometry);

    galerkinState->relLinkErrorThreshold = (float)userErrorThreshold;

    // Push received radiance down the hierarchy to the leaf elements, where
    // it is multiplied with the reflectivity and the self-emitted radiance added,
    // and finally pulls back up for a consistent multi-resolution representation
    // of radiance over all levels
    basisGalerkinPushPullRadiance(galerkinState->topCluster, galerkinState);

    if ( galerkinState->importanceDriven ) {
        gatheringPushPullPotential(galerkinState->topCluster, 0.0);
    }

    // No visualisation with ambient term for gathering radiosity algorithms
    galerkinState->ambientRadiance.clear();

    // Update the display colors of the patches
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        GalerkinRadianceMethod::recomputePatchColor(scenePatches->get(i));
    }

    return false; // Never done
}
