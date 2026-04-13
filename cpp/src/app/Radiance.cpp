/**
Stuff common to all radiance methods
*/

#include "java/util/ArrayList.txx"
#include "galerkin/GalerkinGeometry.h"
#include "galerkin/GalerkinRadianceMethod.h"
#include "app/options/OptionsGroupRadiance.h"
#include "app/Radiance.h"

/**
This routine sets the current radiance method to be used + initializes
*/
void
Radiance::setRadianceMethod(RadianceMethod *radianceMethod, Scene *scene, ToneMappingContext *toneMapOptions) {
    if ( radianceMethod != nullptr ) {
        radianceMethod->terminate(scene->patchList);
        // Until we have radiance data convertors, we dispose of the old data and
        // allocate new data for the new method
        for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
            radianceMethod->destroyPatchData(scene->patchList->get(i));
        }
        for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
            radianceMethod->createPatchData(scene->patchList->get(i));
        }
        if ( radianceMethod->className == RadianceMethodAlgorithm::GALERKIN ) {
            GalerkinGeometry::validateSceneForGalerkin(scene);
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
        RadianceMethod **newRadianceMethod
#ifdef RAYTRACING_ENABLED
        ,
        StochasticRelaxation &stochasticRelaxationState,
        ElementHierarchyState &elementHierarchyState,
        StochasticRadiosityBasisState &stochasticRadiosityBasisState,
        PhotonMapState &photonMapState,
        PhotonMapConfig &photonMapConfig,
        RayMatterState &rayMatterState,
        BidirectionalPathTracingState &bidirectionalPathState,
        StochasticRayTracingState &stochasticRayTracingState
#endif
        )
{
    OptionsGroupRadiance::parse(
        argc,
        argv,
        newRadianceMethod
#ifdef RAYTRACING_ENABLED
        ,
        stochasticRelaxationState,
        elementHierarchyState,
        stochasticRadiosityBasisState,
        photonMapState,
        photonMapConfig,
        rayMatterState,
        bidirectionalPathState,
        stochasticRayTracingState
#endif
        );

    if ( *newRadianceMethod != nullptr ) {
        if ( (*newRadianceMethod)->className == RadianceMethodAlgorithm::GALERKIN ) {
            GalerkinRadianceMethod *galerkinRadianceMethod = static_cast<GalerkinRadianceMethod *>(*newRadianceMethod);
            galerkinRadianceMethod->setStrategy();
        }
        (*newRadianceMethod)->parseOptions(argc, argv);
    }
}
