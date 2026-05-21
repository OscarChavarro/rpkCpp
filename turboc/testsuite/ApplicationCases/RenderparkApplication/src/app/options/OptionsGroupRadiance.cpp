#include <string.h>

#include "vsdk/java/lang/System.h"
#include "app/options/OptionsGroupRayMatter.h"
#include "app/options/OptionsGroupBidirectionalRaytracing.h"
#include "app/options/OptionsGroupGalerkin.h"
#include "app/options/OptionsGroupPhotonMap.h"
#include "app/options/OptionsGroupRadianceMethod.h"
#include "app/options/OptionsGroupRadiance.h"
#include "app/options/OptionsGroupRandomWalkRadiosity.h"
#include "app/options/OptionsGroupStochasticRaytracing.h"
#include "app/options/OptionsGroupStochasticRelaxationRadiosity.h"
#include "vsdk/galerkin/GalerkinRadianceMethod.h"
#include "vsdk/raycasting/stochasticRaytracing/StochasticRelaxation.h"

#ifdef RAYTRACING_ENABLED
    #include "vsdk/raycasting/photonMap/PhotonMapRadianceMethod.h"
    #include "vsdk/raycasting/stochasticRaytracing/StochasticJacobiRadianceMethod.h"
    #include "vsdk/raycasting/stochasticRaytracing/RandomWalkRadianceMethod.h"
#endif

void
OptionsGroupRadiance::selectRadianceMethod(
    const char *name,
    RadianceMethod **newRadianceMethod,
    StochasticRelaxation &stochasticRelaxationState,
    ElementHierarchyState &elementHierarchyState,
    StochasticRadiosityBasisState &stochasticRadiosityBasisState,
    PhotonMapState &photonMapState,
    PhotonMapConfig &photonMapConfig)
{
    if ( name != NULL && name[0] != '\0' ) {
        if ( *newRadianceMethod != NULL ) {
            delete *newRadianceMethod;
            *newRadianceMethod = NULL;
        }

        if ( strncasecmp(name, "Galerkin", 4) == 0 ) {
            *newRadianceMethod = new GalerkinRadianceMethod();
        }
#ifdef RAYTRACING_ENABLED
        else if ( strncasecmp(name, "PMAP", 4) == 0 ) {
            *newRadianceMethod = new PhotonMapRadianceMethod(photonMapState, photonMapConfig);
        } else if ( strncasecmp(name, "StochJacobi", 4) == 0 ) {
            *newRadianceMethod = new StochasticJacobiRadianceMethod(
                stochasticRelaxationState,
                elementHierarchyState,
                stochasticRadiosityBasisState);
        } else if ( strncasecmp(name, "RandomWalk", 4) == 0 ) {
            *newRadianceMethod = new RandomWalkRadianceMethod(
                stochasticRelaxationState,
                elementHierarchyState,
                stochasticRadiosityBasisState);
        }
#endif
    }
}

void
OptionsGroupRadiance::parse(
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
    char radianceMethodsString[RADIANCE_METHODS_STRING_LENGTH];
    radianceMethodsString[0] = '\0';

    OptionsGroupRadianceMethod::radianceMethodParseOptions(argc, argv, radianceMethodsString);

    OptionsGroupRadiance::selectRadianceMethod(
        radianceMethodsString,
        newRadianceMethod,
        stochasticRelaxationState,
        elementHierarchyState,
        stochasticRadiosityBasisState,
        photonMapState,
        photonMapConfig);

    if ( *newRadianceMethod == NULL ) {
#ifdef RAYTRACING_ENABLED
        System::err.printf(
            "ERROR: You must select a radiance mode using '-radiance-method'. "
            "Supported values: Galerkin, PMAP, StochJacobi, RandomWalk.\n");
#else
        System::err.printf(
            "ERROR: You must select a radiance mode using '-radiance-method'. "
            "Supported value: Galerkin.\n");
#endif
        System::err.flush();
        System::exit(1);
    }

#ifdef RAYTRACING_ENABLED
    OptsGrpStochRelaxRad::parse(argc, argv, stochasticRelaxationState, elementHierarchyState);
    OptionsGroupRandomWalkRadiosity::parse(argc, argv, stochasticRelaxationState);
    OptionsGroupRayMatter::rayMattingParseOptions(argc, argv, rayMatterState);
    OptsGrpBidirRaytr::parse(argc, argv, bidirectionalPathState);
    OptsGrpStochRaytr::parse(argc, argv, stochasticRayTracingState);
    OptionsGroupPhotonMap::parse(argc, argv, photonMapState);
#endif

    OptionsGroupGalerkin::galerkinParseOptions(argc, argv);
}
