/**
Stuff common to all radiance methods
*/

#include "skin/radianceinterfaces.h"
#include "common/error.h"
#include "scene/scene.h"
#include "shared/options.h"
#include "shared/defaults.h"
#include "GALERKIN/GalerkinRadiosity.h"
#include "raycasting/stochasticRaytracing/mcrad.h"
#include "PHOTONMAP/PhotonMapRadiosity.h"

// Composes explanation for -radiance command line option
static char globalRadianceMethodsString[1000];

// Table of available radiance methods
RADIANCEMETHOD *GLOBAL_radiance_radianceMethods[] = {
    &GalerkinRadiosity,
    &StochasticRelaxationRadiosity,
    &GLOBAL_stochasticRaytracing_randomWalkRadiosity,
    &GLOBAL_photonMapMethods,
    (RADIANCEMETHOD *) nullptr
};

// Current radiance method handle
RADIANCEMETHOD *GLOBAL_radiance_currentRadianceMethodHandle = (RADIANCEMETHOD *) nullptr;

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

static CMDLINEOPTDESC globalRadianceOptions[] = {
        {"-radiance-method", 4, Tstring,  nullptr, radianceMethodOption,
                globalRadianceMethodsString},
        {nullptr,               0, TYPELESS, nullptr, DEFAULT_ACTION,
                nullptr}
};

/**
This routine sets the current radiance method to be used + initializes
*/
void
setRadianceMethod(RADIANCEMETHOD *newmethod) {
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
    GLOBAL_radiance_currentRadianceMethodHandle = newmethod;
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
    sprintf(str, "\
-radiance-method <method>: Set world-space radiance computation method\n%n",
            &n);
    str += n;
    sprintf(str, "\tmethods: %-20.20s %s%s\n%n",
            "none", "no world-space radiance computation",
            !GLOBAL_radiance_currentRadianceMethodHandle ? " (default)" : "", &n);
    str += n;
    ForAllRadianceMethods(method)
                {
                    sprintf(str, "\t         %-20.20s %s%s\n%n",
                            method->shortName, method->fullName,
                            GLOBAL_radiance_currentRadianceMethodHandle == method ? " (default)" : "", &n);
                    str += n;
                }
    EndForAll;
    *(str - 1) = '\0';    /* discard last newline character */
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
