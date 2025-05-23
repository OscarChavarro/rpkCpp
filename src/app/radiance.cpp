/**
Stuff common to all radiance methods
*/

#include <cstring>

#include "java/util/ArrayList.txx"
#include "common/RenderOptions.h"
#include "GALERKIN/GalerkinRadianceMethod.h"
#include "app/radiance.h"
#include "app/commandLine.h"

#ifdef RAYTRACING_ENABLED
    #include "PHOTONMAP/PhotonMapRadianceMethod.h"
    #include "raycasting/stochasticRaytracing/StochasticJacobiRadianceMethod.h"
    #include "raycasting/stochasticRaytracing/RandomWalkRadianceMethod.h"
#endif

// Composes explanation for -radiance command line option
static const int  STRING_LENGTH = 1000;
static char globalRadianceMethodsString[STRING_LENGTH];

/**
This routine sets the current radiance method to be used + initializes
*/
void
setRadianceMethod(RadianceMethod *radianceMethod, Scene *scene) {
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
        radianceMethod->initialize(scene);
    }
}

static void
selectRadianceMethod(const int *argc, char **argv, RadianceMethod **newRadianceMethod) {
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
            *newRadianceMethod = new PhotonMapRadianceMethod();
        } else if ( strncasecmp(name, "StochJacobi", 4) == 0 ) {
            *newRadianceMethod = new StochasticJacobiRadianceMethod();
        } else if ( strncasecmp(name, "RandomWalk", 4) == 0 ) {
            *newRadianceMethod = new RandomWalkRadianceMethod();
        }
#endif
    }
}

/**
Parses (and consumes) command line options for radiance
computation
*/
void
radianceParseOptions(int *argc, char **argv, RadianceMethod **newRadianceMethod) {
    selectRadianceMethod(argc, argv, newRadianceMethod);
    radianceMethodParseOptions(argc, argv, globalRadianceMethodsString);

#ifdef RAYTRACING_ENABLED
    stochasticRelaxationRadiosityParseOptions(argc, argv);
    randomWalkRadiosityParseOptions(argc, argv);
    rayMattingParseOptions(argc, argv);
    biDirectionalPathParseOptions(argc, argv);
    stochasticRayTracerParseOptions(argc, argv);
    photonMapParseOptions(argc, argv);
#endif

    galerkinParseOptions(argc, argv);

    if ( *newRadianceMethod != nullptr ) {
        if ( (*newRadianceMethod)->className == RadianceMethodAlgorithm::GALERKIN ) {
            GalerkinRadianceMethod *galerkinRadianceMethod = (GalerkinRadianceMethod *)(*newRadianceMethod);
            galerkinRadianceMethod->setStrategy();
        }
        (*newRadianceMethod)->parseOptions(argc, argv);
    }
}
