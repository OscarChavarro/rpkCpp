/**
Stuff common to all radiance methods
*/

#include "java/util/ArrayList.txx"
#include "vsdk/galerkin/GalerkinRadianceMethod.h"
#include "app/options/OptionsGroupRadiance.h"
#include "app/Radiance.h"

/**
This routine sets the current radiance method to be used + initializes
*/
void
Radiance::setRadianceMethod(RadianceMethod *radianceMethod, Scene *scene, ToneMappingContext *toneMapOptions) {
    if ( radianceMethod != NULL ) {
        radianceMethod->terminate(scene->patchList);
        // Until we have radiance data convertors, we dispose of the old data and
        // allocate new data for the new method
        for ( int i = 0; scene->patchList != NULL && i < scene->patchList->size(); i++ ) {
            radianceMethod->destroyPatchData(scene->patchList->get(i));
        }
        for ( int i = 0; scene->patchList != NULL && i < scene->patchList->size(); i++ ) {
            radianceMethod->createPatchData(scene->patchList->get(i));
        }
        radianceMethod->initialize(scene, toneMapOptions);
    }
}

/**
Parses (and consumes) command line options for radiance
computation
*/
void
Radiance::radianceParseOptions(
        int *argc,
        char **argv,
        RadianceMethod **newRadianceMethod,
        StochasticRelaxation &stochasticRelaxationState,
        ElementHierarchyState &elementHierarchyState,
        StochasticRadiosityBasisState &stochasticRadiosityBasisState,
        PhotonMapState &photonMapState,
        PhotonMapConfig &photonMapConfig,
        RayMatterState &rayMatterState,
        BidirectionalPathTracingState &bidirectionalPathState,
        StochasticRayTracingState &stochasticRayTracingState)
{
    OptionsGroupRadiance::parse(
        argc,
        argv,
        newRadianceMethod,
        stochasticRelaxationState,
        elementHierarchyState,
        stochasticRadiosityBasisState,
        photonMapState,
        photonMapConfig,
        rayMatterState,
        bidirectionalPathState,
        stochasticRayTracingState);

    if ( *newRadianceMethod != NULL ) {
        if ( (*newRadianceMethod)->className == GALERKIN ) {
            GalerkinRadianceMethod *galerkinRadianceMethod = ((GalerkinRadianceMethod *)(*newRadianceMethod));
            galerkinRadianceMethod->setStrategy();
        }
        (*newRadianceMethod)->parseOptions(argc, argv);
    }
}
