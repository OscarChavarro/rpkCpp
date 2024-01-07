// pmapoptions.C

#include "PHOTONMAP/pmapoptions.h"

// Global pmap options
CPmapState pmapstate;

// Command line options -- TODO expand...
static CMDLINEOPTDESC pmapOptions[] = {
        {"-pmap-do-global",       9,  Tbool,    &pmapstate.doGlobalMap,           DEFAULT_ACTION,
                "-pmap-do-global <true|false> : Trace photons for the global map"},
        {"-pmap-global-paths",    9,  Tint,     &pmapstate.gpaths_per_iteration,  DEFAULT_ACTION,
                "-pmap-global-paths <number> : Number of paths per iteration for the global map"},
        {"-pmap-g-preirradiance", 11, Tbool,    &pmapstate.precomputeGIrradiance, DEFAULT_ACTION,
                "-pmap-g-preirradiance <true|false> : Use irradiance precomputation for global map"},
        {"-pmap-do-caustic",      9,  Tbool,    &pmapstate.doCausticMap,          DEFAULT_ACTION,
                "-pmap-do-caustic <true|false> : Trace photons for the caustic map"},
        {"-pmap-caustic-paths",   9,  Tint,     &pmapstate.cpaths_per_iteration,  DEFAULT_ACTION,
                "-pmap-caustic-paths <number> : Number of paths per iteration for the caustic map"},
        {"-pmap-render-hits",     9,  Tsettrue, &pmapstate.renderImage,           DEFAULT_ACTION,
                "-pmap-render-hits: Show photon hits on screen"},
        {"-pmap-recon-gphotons",  9,  Tint,     &pmapstate.reconGPhotons,         DEFAULT_ACTION,
                "-pmap-recon-cphotons <number> : Number of photons to use in reconstructions (global map)"},
        {"-pmap-recon-iphotons",  9,  Tint,     &pmapstate.reconCPhotons,         DEFAULT_ACTION,
                "-pmap-recon-photons <number> : Number of photons to use in reconstructions (caustic map)"},
        {"-pmap-recon-photons",   9,  Tint,     &pmapstate.reconIPhotons,         DEFAULT_ACTION,
                "-pmap-recon-photons <number> : Number of photons to use in reconstructions (importance)"},
        {"-pmap-balancing",       9,  Tbool,    &pmapstate.balanceKDTree,         DEFAULT_ACTION,
                "-pmap-balancing <true|false> : Balance KD Tree before raytracing"},
        {nullptr,                    0,  TYPELESS, nullptr,                             DEFAULT_ACTION, nullptr}
};


void ParsePmapOptions(int *argc, char **argv) {
    parseOptions(pmapOptions, argc, argv);
}

void PrintPmapOptions(FILE *fp) {
}

CPmapState::CPmapState() {
    // One time state initialisations
    rcScreen = nullptr;

    // Set other defaults, that can be reset multiple times
    Defaults();
}

void
CPmapState::Defaults() {
    // This is the only place where default values may be given...
    doCausticMap = true;
    cpaths_per_iteration = 20000;

    doGlobalMap = true;
    gpaths_per_iteration = 10000;
    precomputeGIrradiance = true;

    renderImage = false;

    reconGPhotons = 80;
    reconCPhotons = 80;
    reconIPhotons = 200;
    distribPhotons = 20;

    balanceKDTree = true;
    usePhotonMapSampler = false;

    densityControl = NO_DENSITY_CONTROL;
    acceptPdfType = STEP;

    constantRD = 10000;

    minimumImpRD = 1;
    doImportanceMap = true;
    ipaths_per_iteration = 10000;

    importanceOption = USE_IMPORTANCE;

    cImpScale = 25.0;
    gImpScale = 1.0;

    cornerScatter = true;
    gThreshold = 1000;

    falseColMax = 10000;
    falseColLog = false;
    falseColMono = false;

    radianceReturn = GLOBAL_RADIANCE;


    minimumLightPathDepth = 0;
    maximumLightPathDepth = 7;
    maxCombinedLength = 7;

    doStats = false;
}
