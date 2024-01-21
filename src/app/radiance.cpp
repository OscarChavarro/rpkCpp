/**
Stuff common to all radiance methods
*/

#include "common/error.h"
#include "scene/scene.h"
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

    ForAllRadianceMethods(method)
                {
                    if ( strncasecmp(name, method->shortName, method->nameAbbrev) == 0 ) {
                        setRadianceMethod(method);
                        return;
                    }
                }
    EndForAll;

    if ( strncasecmp(name, "none", 4) == 0 ) {
        setRadianceMethod(nullptr);
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
setRadianceMethod(RADIANCEMETHOD *newMethod) {
    if ( GLOBAL_radiance_currentRadianceMethodHandle ) {
        GLOBAL_radiance_currentRadianceMethodHandle->Terminate();
        // Until we have radiance data convertors, we dispose of the old data and
        // allocate new data for the new method
        if ( GLOBAL_radiance_currentRadianceMethodHandle->DestroyPatchData ) {
            for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
                GLOBAL_radiance_currentRadianceMethodHandle->DestroyPatchData(window->patch);
            }
        }
    }
    GLOBAL_radiance_currentRadianceMethodHandle = newMethod;
    if ( GLOBAL_radiance_currentRadianceMethodHandle != nullptr ) {
        if ( GLOBAL_radiance_currentRadianceMethodHandle->CreatePatchData ) {
            for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
                GLOBAL_radiance_currentRadianceMethodHandle->CreatePatchData(window->patch);
            }
        }
        GLOBAL_radiance_currentRadianceMethodHandle->Initialize();
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
    ForAllRadianceMethods(method)
                {
                    snprintf(str, STRING_LENGTH, "\t         %-20.20s %s%s\n%n",
                            method->shortName, method->fullName,
                            GLOBAL_radiance_currentRadianceMethodHandle == method ? " (default)" : "", &n);
                    str += n;
                }
    EndForAll;
    *(str - 1) = '\0'; // Discard last newline character
}

void
radianceDefaults() {
    ForAllRadianceMethods(method)
                {
                    method->Defaults();
                    if ( strncasecmp(DEFAULT_RADIANCE_METHOD, method->shortName, method->nameAbbrev) == 0 ) {
                        setRadianceMethod(method);
                    }
                }
    EndForAll;
    makeRadianceMethodsString();
}

void
parseRadianceOptions(int *argc, char **argv) {
    parseOptions(globalRadianceOptions, argc, argv);

    ForAllRadianceMethods(method)
                {
                    method->ParseOptions(argc, argv);
                }
    EndForAll;
}
