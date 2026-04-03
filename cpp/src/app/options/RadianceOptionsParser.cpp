#include <cstring>

#include "java/lang/System.h"
#include "app/options/CommandLine.h"
#include "app/options/RadianceOptionsParser.h"
#include "galerkin/GalerkinRadianceMethod.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

#ifdef RAYTRACING_ENABLED
    #include "raycasting/photonMap/PhotonMapRadianceMethod.h"
    #include "raycasting/stochasticRaytracing/StochasticJacobiRadianceMethod.h"
    #include "raycasting/stochasticRaytracing/RandomWalkRadianceMethod.h"
#endif

void
RadianceOptionsParser::selectRadianceMethod(
    const int *argc,
    char **argv,
    RadianceMethod **newRadianceMethod,
    StochasticRelaxation &stochasticRelaxationState,
    ElementHierarchyState &elementHierarchyState,
    StochasticRadiosityBasisState &stochasticRadiosityBasisState,
    PhotonMapState &photonMapState,
    PhotonMapConfig &photonMapConfig)
{
    bool getNext = false;
    const char *name = nullptr;
    for ( int i = 0; i < *argc; i++ ) {
        if ( strcmp(argv[i], "-radiance-method") == 0 ) {
            getNext = true;
            continue;
        } else if ( getNext ) {
            name = argv[i];
            break;
        }
    }

    if ( name != nullptr ) {
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
#endif
    }
}

void
RadianceOptionsParser::parse(
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
    StochasticRayTracingState &stochasticRayTracingState,
    OptionsType &optionTypes)
{
    char radianceMethodsString[RADIANCE_METHODS_STRING_LENGTH];

    RadianceOptionsParser::selectRadianceMethod(
        argc,
        argv,
        newRadianceMethod,
        stochasticRelaxationState,
        elementHierarchyState,
        stochasticRadiosityBasisState,
        photonMapState,
        photonMapConfig);
    CommandLine::radianceMethodParseOptions(argc, argv, radianceMethodsString, optionTypes);

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
    CommandLine::stochasticRelaxationRadiosityParseOptions(argc, argv, stochasticRelaxationState, elementHierarchyState, optionTypes);
    CommandLine::randomWalkRadiosityParseOptions(argc, argv, stochasticRelaxationState, optionTypes);
    CommandLine::rayMattingParseOptions(argc, argv, rayMatterState, optionTypes);
    CommandLine::biDirectionalPathParseOptions(argc, argv, bidirectionalPathState, optionTypes);
    CommandLine::stochasticRayTracerParseOptions(argc, argv, stochasticRayTracingState, optionTypes);
    CommandLine::photonMapParseOptions(argc, argv, photonMapState, optionTypes);
#endif

    CommandLine::galerkinParseOptions(argc, argv, optionTypes);
}
