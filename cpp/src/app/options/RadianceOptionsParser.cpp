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
    const char *name,
    RadianceMethod **newRadianceMethod,
    StochasticRelaxation &stochasticRelaxationState,
    ElementHierarchyState &elementHierarchyState,
    StochasticRadiosityBasisState &stochasticRadiosityBasisState,
    PhotonMapState &photonMapState,
    PhotonMapConfig &photonMapConfig)
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
    StochasticRayTracingState &stochasticRayTracingState)
{
    char radianceMethodsString[RADIANCE_METHODS_STRING_LENGTH];
    radianceMethodsString[0] = '\0';

    CommandLine::radianceMethodParseOptions(argc, argv, radianceMethodsString);

    RadianceOptionsParser::selectRadianceMethod(
        radianceMethodsString,
        newRadianceMethod,
        stochasticRelaxationState,
        elementHierarchyState,
        stochasticRadiosityBasisState,
        photonMapState,
        photonMapConfig);

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
    CommandLine::stochasticRelaxationRadiosityParseOptions(argc, argv, stochasticRelaxationState, elementHierarchyState);
    CommandLine::randomWalkRadiosityParseOptions(argc, argv, stochasticRelaxationState);
    CommandLine::rayMattingParseOptions(argc, argv, rayMatterState);
    CommandLine::biDirectionalPathParseOptions(argc, argv, bidirectionalPathState);
    CommandLine::stochasticRayTracerParseOptions(argc, argv, stochasticRayTracingState);
    CommandLine::photonMapParseOptions(argc, argv, photonMapState);
#endif

    CommandLine::galerkinParseOptions(argc, argv);
}
