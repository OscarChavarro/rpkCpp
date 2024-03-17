/**
Stuff common to all radiance methods
*/

#include <cstring>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/options.h"
#include "common/RenderOptions.h"
#include "GALERKIN/GalerkinRadianceMethod.h"

#ifdef RAYTRACING_ENABLED
    #include "PHOTONMAP/PhotonMapRadianceMethod.h"
    #include "raycasting/stochasticRaytracing/StochasticJacobiRadianceMethod.h"
    #include "raycasting/stochasticRaytracing/RandomWalkRadianceMethod.h"
#endif

// Composes explanation for -radiance command line option
#define STRING_LENGTH 1000
static char globalRadianceMethodsString[STRING_LENGTH];

// Current radiance method handle
java::ArrayList<Patch *> *GLOBAL_scenePatches = nullptr;

static void
radianceMethodOption(void *value) {
    char *name = *(char **) value;

    setRadianceMethod(GLOBAL_radiance_selectedRadianceMethod, GLOBAL_scenePatches);

    if ( strncasecmp(name, "none", 4) == 0 ) {
        setRadianceMethod(nullptr, GLOBAL_scenePatches);
    } else {
        logError(nullptr, "Invalid world-space radiance method name '%s'", name);
    }
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
setRadianceMethod(RadianceMethod *newMethod, java::ArrayList<Patch *> *scenePatches) {
    if ( newMethod != nullptr ) {
        newMethod->terminate(scenePatches);
        // Until we have radiance data convertors, we dispose of the old data and
        // allocate new data for the new method
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            newMethod->destroyPatchData(scenePatches->get(i));
        }
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            newMethod->createPatchData(scenePatches->get(i));
        }
        newMethod->initialize(scenePatches);
    }
}

void
radianceDefaults(java::ArrayList<Patch *> *scenePatches) {
    setRadianceMethod(GLOBAL_radiance_selectedRadianceMethod, scenePatches);
}

static void
selectRadianceMethod(const int *argc, char **argv) {
    bool getNext = false;
    char *name = nullptr;
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
        if ( GLOBAL_radiance_selectedRadianceMethod != nullptr ) {
            delete GLOBAL_radiance_selectedRadianceMethod;
            GLOBAL_radiance_selectedRadianceMethod = nullptr;
        }

        if ( strncasecmp(name, "Galerkin", 4) == 0 ) {
            GLOBAL_radiance_selectedRadianceMethod = new GalerkinRadianceMethod();
        }
#ifdef RAYTRACING_ENABLED
        else if ( strncasecmp(name, "PMAP", 4) == 0 ) {
            GLOBAL_radiance_selectedRadianceMethod = new PhotonMapRadianceMethod();
        } else if ( strncasecmp(name, "StochJacobi", 4) == 0 ) {
            GLOBAL_radiance_selectedRadianceMethod = new StochasticJacobiRadianceMethod();
        } else if ( strncasecmp(name, "RandomWalk", 4) == 0 ) {
            GLOBAL_radiance_selectedRadianceMethod = new RandomWalkRadianceMethod();
        }
    }
#endif
}

/**
Parses (and consumes) command line options for radiance
computation
*/
void
parseRadianceOptions(int *argc, char **argv) {
    selectRadianceMethod(argc, argv);
    parseGeneralOptions(globalRadianceOptions, argc, argv);
    if ( GLOBAL_radiance_selectedRadianceMethod != nullptr ) {
        GLOBAL_radiance_selectedRadianceMethod->parseOptions(argc, argv);
    }
}
