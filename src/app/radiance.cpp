/**
Stuff common to all radiance methods
*/

#include <cstring>

#include "java/util/ArrayList.txx"
#include "common/options.h"
#include "common/RenderOptions.h"
#include "GALERKIN/GalerkinRadianceMethod.h"
#include "app/radiance.h"

#ifdef RAYTRACING_ENABLED
    #include "PHOTONMAP/PhotonMapRadianceMethod.h"
    #include "raycasting/stochasticRaytracing/StochasticJacobiRadianceMethod.h"
    #include "raycasting/stochasticRaytracing/RandomWalkRadianceMethod.h"
#endif

// Composes explanation for -radiance command line option
#define STRING_LENGTH 1000
static char globalRadianceMethodsString[STRING_LENGTH];

static void
radianceMethodOption(void * /*value*/) {
}

static CommandLineOptionDescription globalRadianceOptions[] = {
        {"-radiance-method", 4, Tstring,  nullptr, radianceMethodOption,
                globalRadianceMethodsString},
        {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

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

void
radianceDefaults(RadianceMethod *radianceMethod, Scene *scene) {
    setRadianceMethod(radianceMethod, scene);
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
    parseGeneralOptions(globalRadianceOptions, argc, argv);
    if ( *newRadianceMethod != nullptr ) {
        (*newRadianceMethod)->parseOptions(argc, argv);
    }
}
