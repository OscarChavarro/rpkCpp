#include <cstring>
#include <cstring>

#include "java/lang/System.h"
#include "app/options/OptionsGroupGalerkin.h"
#include "app/options/OptionsGroupRadianceMethod.h"
#include "app/options/OptionsGroupRadiance.h"
#include "galerkin/GalerkinRadianceMethod.h"

#ifdef RAYTRACING_ENABLED
    #include "app/options/OptionsGroupRayMatter.h"
    #include "app/options/OptionsGroupBidirectionalRaytracing.h"
    #include "app/options/OptionsGroupPhotonMap.h"
    #include "app/options/OptionsGroupRandomWalkRadiosity.h"
    #include "app/options/OptionsGroupStochasticRaytracing.h"
    #include "app/options/OptionsGroupStochasticRelaxationRadiosity.h"
    #include "raycasting/stochasticRaytracing/StochasticRelaxation.h"
    #include "raycasting/photonMap/PhotonMapRadianceMethod.h"
    #include "raycasting/stochasticRaytracing/StochasticJacobiRadianceMethod.h"
    #include "raycasting/stochasticRaytracing/RandomWalkRadianceMethod.h"
#endif

void
OptionsGroupRadiance::selectRadianceMethod(
    const char *name,
    RadianceMethod **newRadianceMethod
#ifdef RAYTRACING_ENABLED
    ,
    StochasticRelaxation &stochasticRelaxationState,
    ElementHierarchyState &elementHierarchyState,
    StochasticRadiosityBasisState &stochasticRadiosityBasisState,
    PhotonMapState &photonMapState,
    PhotonMapConfig &photonMapConfig
#endif
    )
{
    if ( name != nullptr && name[0] != '\0' ) {
        if ( *newRadianceMethod != nullptr ) {
            delete *newRadianceMethod;
            *newRadianceMethod = nullptr;
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
#else
        else {
            java::System::err.printf(
                "ERROR: Radiance method '%s' requires raytracing support. Rebuild with CMake flag '-DWITH_RAYTRACING=ON'.\n",
                name);
            java::System::err.flush();
            java::System::exit(1);
        }
#endif
    }
}

void
OptionsGroupRadiance::parse(
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
    char radianceMethodsString[RADIANCE_METHODS_STRING_LENGTH];
    radianceMethodsString[0] = '\0';

    OptionsGroupRadianceMethod::radianceMethodParseOptions(argc, argv, radianceMethodsString);

    OptionsGroupRadiance::selectRadianceMethod(
        radianceMethodsString,
        newRadianceMethod
#ifdef RAYTRACING_ENABLED
        ,
        stochasticRelaxationState,
        elementHierarchyState,
        stochasticRadiosityBasisState,
        photonMapState,
        photonMapConfig
#endif
        );

    if ( *newRadianceMethod == nullptr ) {
#ifdef RAYTRACING_ENABLED
        java::System::err.printf(
            "ERROR: You must select a radiance mode using '-radiance-method'. "
            "Supported values: Galerkin, PMAP, StochJacobi, RandomWalk.\n");
#else
        java::System::err.printf(
            "ERROR: You must select a radiance mode using '-radiance-method'. "
            "Supported value: Galerkin.\n");
#endif
        java::System::err.flush();
        java::System::exit(1);
    }

#ifdef RAYTRACING_ENABLED
    OptionsGroupStochasticRelaxationRadiosity::parse(argc, argv, stochasticRelaxationState, elementHierarchyState);
    OptionsGroupRandomWalkRadiosity::parse(argc, argv, stochasticRelaxationState);
    OptionsGroupRayMatter::rayMattingParseOptions(argc, argv, rayMatterState);
    OptionsGroupBidirectionalRaytracing::parse(argc, argv, bidirectionalPathState);
    OptionsGroupStochasticRaytracing::parse(argc, argv, stochasticRayTracingState);
    OptionsGroupPhotonMap::parse(argc, argv, photonMapState);
#endif

    OptionsGroupGalerkin::galerkinParseOptions(argc, argv);
}
