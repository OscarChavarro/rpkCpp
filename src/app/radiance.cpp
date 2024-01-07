/* radiance.c: stuff common to all radiance methods */

#include <cstring>

#include "skin/radianceinterfaces.h"
#include "common/error.h"
#include "scene/scene.h"
#include "shared/options.h"
#include "shared/defaults.h"
#include "GALERKIN/GalerkinRadiosity.h"
#include "raycasting/stochasticRaytracing/mcrad.h"
#include "PHOTONMAP/PhotonMapRadiosity.h"

/* table of available radiance methods */
RADIANCEMETHOD *RadianceMethods[] = {
        &GalerkinRadiosity,
        &StochasticRelaxationRadiosity,
        &RandomWalkRadiosity,
        &Pmap,
        (RADIANCEMETHOD *) nullptr
};

/* current radiance method handle */
RADIANCEMETHOD *GLOBAL_radiance_currentRadianceMethodHandle = (RADIANCEMETHOD *) nullptr;

static void
RadianceMethodOption(void *value) {
    char *name = *(char **) value;

    ForAllRadianceMethods(method)
                {
                    if ( strncasecmp(name, method->shortName, method->nameAbbrev) == 0 ) {
                        SetRadianceMethod(method);
                        return;
                    }
                }
    EndForAll;

    if ( strncasecmp(name, "none", 4) == 0 ) {
        SetRadianceMethod(nullptr);
    } else {
        logError(nullptr, "Invalid world-space radiance method name '%s'", name);
    }
}

/* composes explanation for -radiance command line option */
static char radiance_methods_string[1000];

static CMDLINEOPTDESC radianceOptions[] = {
        {"-radiance-method", 4, Tstring,  nullptr, RadianceMethodOption,
                radiance_methods_string},
        {nullptr,               0, TYPELESS, nullptr, DEFAULT_ACTION,
                nullptr}
};

/* This routine sets the current radiance method to be used + initializes */
void
SetRadianceMethod(RADIANCEMETHOD *newmethod) {
    if ( GLOBAL_radiance_currentRadianceMethodHandle ) {
        GLOBAL_radiance_currentRadianceMethodHandle->Terminate();
        /* until we have radiance data convertors, we dispose of the old data and
         * allocate new data for the new method. */
        if ( GLOBAL_radiance_currentRadianceMethodHandle->DestroyPatchData ) PatchListIterate(GLOBAL_scene_patches, GLOBAL_radiance_currentRadianceMethodHandle->DestroyPatchData);
    }
    GLOBAL_radiance_currentRadianceMethodHandle = newmethod;
    if ( GLOBAL_radiance_currentRadianceMethodHandle ) {
        if ( GLOBAL_radiance_currentRadianceMethodHandle->CreatePatchData ) PatchListIterate(GLOBAL_scene_patches, GLOBAL_radiance_currentRadianceMethodHandle->CreatePatchData);
        GLOBAL_radiance_currentRadianceMethodHandle->Initialize();
    }
}

static void
make_radiance_methods_string() {
    char *str = radiance_methods_string;
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
RadianceDefaults() {
    ForAllRadianceMethods(method)
                {
                    method->Defaults();
                    if ( strncasecmp(DEFAULT_RADIANCE_METHOD, method->shortName, method->nameAbbrev) == 0 ) {
                        SetRadianceMethod(method);
                    }
                }
    EndForAll;
    make_radiance_methods_string();
}

void
ParseRadianceOptions(int *argc, char **argv) {
    parseOptions(radianceOptions, argc, argv);

    ForAllRadianceMethods(method)
                {
                    method->ParseOptions(argc, argv);
                }
    EndForAll;
}
