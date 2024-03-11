/**
Stuff common to all radiance methods
*/

#include <cstring>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/options.h"
#include "common/RenderOptions.h"
#include "GALERKIN/GalerkinRadianceMethod.h"
#include "app/batch.h"

#ifdef RAYTRACING_ENABLED
    #include "PHOTONMAP/PhotonMapRadianceMethod.h"
    #include "raycasting/stochasticRaytracing/mcrad.h"
    #include "raycasting/stochasticRaytracing/StochasticJacobiRadianceMethod.h"
    #include "raycasting/stochasticRaytracing/RandomWalkRadianceMethod.h"
#endif

// Default radiance method: a short name of a radiance method or "none"
#define DEFAULT_RADIANCE_METHOD "none"

// Composes explanation for -radiance command line option
#define STRING_LENGTH 1000
static char globalRadianceMethodsString[STRING_LENGTH];

// Table of available radiance methods
RADIANCEMETHOD *GLOBAL_radiance_radianceMethods[] = {
&GLOBAL_galerkin_radiosity,
    #ifdef RAYTRACING_ENABLED
        &GLOBAL_stochasticRaytracing_stochasticRelaxationRadiosity,
        &GLOBAL_stochasticRaytracing_randomWalkRadiosity,
        &GLOBAL_photonMapMethods,
    #endif
nullptr
};

// Current radiance method handle
RADIANCEMETHOD *GLOBAL_radiance_currentRadianceMethodHandle = nullptr;
java::ArrayList<Patch *> *GLOBAL_scenePatches = nullptr;

static void
radianceMethodOption(void *value) {
    char *name = *(char **) value;

    for ( RADIANCEMETHOD **methodPointer = GLOBAL_radiance_radianceMethods; *methodPointer != nullptr; methodPointer++) {
        RADIANCEMETHOD *method = *methodPointer;
        if ( strncasecmp(name, method->shortName, method->shortNameMinimumLength) == 0 ) {
            setRadianceMethod(method, GLOBAL_scenePatches);
            return;
        }
    }

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
setRadianceMethod(RADIANCEMETHOD *newMethod, java::ArrayList<Patch *> *scenePatches) {
    if ( GLOBAL_radiance_currentRadianceMethodHandle != nullptr && GLOBAL_radiance_selectedRadianceMethod != nullptr ) {
        GLOBAL_radiance_selectedRadianceMethod->terminate(scenePatches);
        // Until we have radiance data convertors, we dispose of the old data and
        // allocate new data for the new method
        if ( GLOBAL_radiance_currentRadianceMethodHandle->destroyPatchData ) {
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                GLOBAL_radiance_currentRadianceMethodHandle->destroyPatchData(scenePatches->get(i));
            }
        }
    }
    GLOBAL_radiance_currentRadianceMethodHandle = newMethod;
    if ( GLOBAL_radiance_currentRadianceMethodHandle != nullptr ) {
        if ( GLOBAL_radiance_currentRadianceMethodHandle->createPatchData ) {
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                GLOBAL_radiance_currentRadianceMethodHandle->createPatchData(scenePatches->get(i));
            }
        }
        GLOBAL_radiance_currentRadianceMethodHandle->initialize(scenePatches);
    }
}

static void
makeRadianceMethodsString() {
    char *str = globalRadianceMethodsString;
    int n;
    snprintf(str, STRING_LENGTH, "-radiance-method <method>: set world-space radiance computation method\n%n",
            &n);
    str += n;
    snprintf(str, STRING_LENGTH, "\tmethods: %-20.20s %s%s\n%n",
            "none", "no world-space radiance computation",
            !GLOBAL_radiance_currentRadianceMethodHandle ? " (default)" : "", &n);
    str += n;
    for ( RADIANCEMETHOD **methodPointer = GLOBAL_radiance_radianceMethods; *methodPointer != nullptr; methodPointer++) {
        RADIANCEMETHOD *method = *methodPointer;
        snprintf(str, STRING_LENGTH, "\t         %-20.20s %s%s\n%n",
                method->shortName, method->fullName,
                GLOBAL_radiance_currentRadianceMethodHandle == method ? " (default)" : "", &n);
        str += n;
    }
    *(str - 1) = '\0'; // Discard last newline character
}

void
radianceDefaults(java::ArrayList<Patch *> *scenePatches) {
    for ( RADIANCEMETHOD **methodPointer = GLOBAL_radiance_radianceMethods; *methodPointer != nullptr; methodPointer++) {
        RADIANCEMETHOD *method = *methodPointer;
        method->defaultValues();
        GLOBAL_radiance_selectedRadianceMethod = new GalerkinRadianceMethod();
        if ( strncasecmp(DEFAULT_RADIANCE_METHOD, method->shortName, method->shortNameMinimumLength) == 0 ) {
            setRadianceMethod(method, scenePatches);
        }
    }
    makeRadianceMethodsString();
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
    parseOptions(globalRadianceOptions, argc, argv);
    for ( RADIANCEMETHOD **methodPointer = GLOBAL_radiance_radianceMethods; *methodPointer != nullptr; methodPointer++) {
        RADIANCEMETHOD *method = *methodPointer;
        method->parseOptions(argc, argv);
    }
}
