/**
Stuff common to all radiance methods
*/

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "shared/defaults.h"
#include "GALERKIN/GalerkinRadiosity.h"
#include "raycasting/stochasticRaytracing/mcrad.h"
#include "PHOTONMAP/PhotonMapRadiosity.h"

// Composes explanation for -radiance command line option
#define STRING_LENGTH 1000
static char globalRadianceMethodsString[STRING_LENGTH];

// Table of available radiance methods
RADIANCEMETHOD *GLOBAL_radiance_radianceMethods[] = {
&GLOBAL_galerkin_radiosity,
&GLOBAL_stochasticRaytracing_stochasticRelaxationRadiosity,
&GLOBAL_stochasticRaytracing_randomWalkRadiosity,
&GLOBAL_photonMapMethods,
nullptr
};

// Current radiance method handle
RADIANCEMETHOD *GLOBAL_radiance_currentRadianceMethodHandle = nullptr;

static void
radianceMethodOption(void *value) {
    char *name = *(char **) value;
    java::ArrayList<Patch *> *scenePatches = convertPatchSetToPatchList(GLOBAL_scene_patches);

    RADIANCEMETHOD **methodpp;
    for ( methodpp = GLOBAL_radiance_radianceMethods; *methodpp != nullptr; methodpp++) {
        RADIANCEMETHOD *method = *methodpp;
        if ( strncasecmp(name, method->shortName, method->nameAbbrev) == 0 ) {
            setRadianceMethod(method, scenePatches);
            return;
        }
    }

    if ( strncasecmp(name, "none", 4) == 0 ) {
        setRadianceMethod(nullptr, scenePatches);
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
    if ( GLOBAL_radiance_currentRadianceMethodHandle ) {
        GLOBAL_radiance_currentRadianceMethodHandle->Terminate();
        // Until we have radiance data convertors, we dispose of the old data and
        // allocate new data for the new method
        if ( GLOBAL_radiance_currentRadianceMethodHandle->DestroyPatchData ) {
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                GLOBAL_radiance_currentRadianceMethodHandle->DestroyPatchData(scenePatches->get(i));
            }
        }
    }
    GLOBAL_radiance_currentRadianceMethodHandle = newMethod;
    if ( GLOBAL_radiance_currentRadianceMethodHandle != nullptr ) {
        if ( GLOBAL_radiance_currentRadianceMethodHandle->CreatePatchData ) {
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                GLOBAL_radiance_currentRadianceMethodHandle->CreatePatchData(scenePatches->get(i));
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
    RADIANCEMETHOD **methodpp;
    for ( methodpp = GLOBAL_radiance_radianceMethods; *methodpp != nullptr; methodpp++) {
        RADIANCEMETHOD *method = *methodpp;
        snprintf(str, STRING_LENGTH, "\t         %-20.20s %s%s\n%n",
                method->shortName, method->fullName,
                GLOBAL_radiance_currentRadianceMethodHandle == method ? " (default)" : "", &n);
        str += n;
    }
    *(str - 1) = '\0'; // Discard last newline character
}

void
radianceDefaults(java::ArrayList<Patch *> *scenePatches) {
    RADIANCEMETHOD **methodpp;
    for ( methodpp = GLOBAL_radiance_radianceMethods; *methodpp != nullptr; methodpp++) {
        RADIANCEMETHOD *method = *methodpp;
        method->Defaults();
        if ( strncasecmp(DEFAULT_RADIANCE_METHOD, method->shortName, method->nameAbbrev) == 0 ) {
            setRadianceMethod(method, scenePatches);
        }
    }
    makeRadianceMethodsString();
}

/**
Parses (and consumes) command line options for radiance
computation
*/
void
parseRadianceOptions(int *argc, char **argv) {
    parseOptions(globalRadianceOptions, argc, argv);

    RADIANCEMETHOD **methodpp;
    for ( methodpp = GLOBAL_radiance_radianceMethods; *methodpp != nullptr; methodpp++) {
        RADIANCEMETHOD *method = *methodpp;
        method->ParseOptions(argc, argv);
    }
}
