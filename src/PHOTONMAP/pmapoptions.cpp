#include "common/options.h"
#include "PHOTONMAP/pmapoptions.h"

PhotonMapState GLOBAL_photonMap_state;

// Command line options
static CommandLineOptionDescription globalPhotonMapOptions[] = {
    {"-pmap-do-global",       9,  Tbool,    &GLOBAL_photonMap_state.doGlobalMap,           DEFAULT_ACTION,
"-pmap-do-global <true|false> : Trace photons for the global map"},
    {"-pmap-global-paths",    9,  Tint,     &GLOBAL_photonMap_state.gPathsPerIteration, DEFAULT_ACTION,
"-pmap-global-paths <number> : Number of paths per iteration for the global map"},
    {"-pmap-g-preirradiance", 11, Tbool,    &GLOBAL_photonMap_state.precomputeGIrradiance, DEFAULT_ACTION,
"-pmap-g-preirradiance <true|false> : Use irradiance precomputation for global map"},
    {"-pmap-do-caustic",      9,  Tbool,    &GLOBAL_photonMap_state.doCausticMap,          DEFAULT_ACTION,
"-pmap-do-caustic <true|false> : Trace photons for the caustic map"},
    {"-pmap-caustic-paths",   9,  Tint,     &GLOBAL_photonMap_state.cPathsPerIteration, DEFAULT_ACTION,
"-pmap-caustic-paths <number> : Number of paths per iteration for the caustic map"},
    {"-pmap-render-hits",     9,  Tsettrue, &GLOBAL_photonMap_state.renderImage,           DEFAULT_ACTION,
"-pmap-render-hits: Show photon hits on screen"},
    {"-pmap-recon-gphotons",  9,  Tint,     &GLOBAL_photonMap_state.reconGPhotons,         DEFAULT_ACTION,
"-pmap-recon-cphotons <number> : Number of photons to use in reconstructions (global map)"},
    {"-pmap-recon-iphotons",  9,  Tint,     &GLOBAL_photonMap_state.reconCPhotons,         DEFAULT_ACTION,
"-pmap-recon-photons <number> : Number of photons to use in reconstructions (caustic map)"},
    {"-pmap-recon-photons",   9,  Tint,     &GLOBAL_photonMap_state.reconIPhotons,         DEFAULT_ACTION,
"-pmap-recon-photons <number> : Number of photons to use in reconstructions (importance)"},
    {"-pmap-balancing",       9,  Tbool,    &GLOBAL_photonMap_state.balanceKDTree,         DEFAULT_ACTION,
"-pmap-balancing <true|false> : Balance KD Tree before raytracing"},
    {nullptr, 0,  TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void
photonMapParseOptions(int *argc, char **argv) {
    parseOptions(globalPhotonMapOptions, argc, argv);
}

PhotonMapState::PhotonMapState():
        doGlobalMap(), gPathsPerIteration(), precomputeGIrradiance(), doCausticMap(), cPathsPerIteration(),
        renderImage(), reconGPhotons(), reconCPhotons(), reconIPhotons(), distribPhotons(), balanceKDTree(),
        usePhotonMapSampler(), densityControl(), importanceOption(), acceptPdfType(), constantRD(), minimumImpRD(),
        doImportanceMap(), iPathsPerIteration(), cImpScale(), gImpScale(), gThreshold(),
        falseColMax(), falseColLog(), falseColMono(), radianceReturn(), minimumLightPathDepth(),
        maximumLightPathDepth(), iterationNumber(), gIterationNumber(), cIterationNumber(),
        i_iteration_nr(), totalCPaths(), totalGPaths(), totalIPaths(), runStopNumber(),
        cpuSecs(), lastClock()
{
    // Set other defaults, that can be reset multiple times
    defaults();
}

void
PhotonMapState::defaults() {
    // This is the only place where default values may be given...
    doCausticMap = true;
    cPathsPerIteration = 20000;

    doGlobalMap = true;
    gPathsPerIteration = 10000;
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
    iPathsPerIteration = 10000;

    importanceOption = USE_IMPORTANCE;

    cImpScale = 25.0;
    gImpScale = 1.0;

    gThreshold = 1000;

    falseColMax = 10000;
    falseColLog = false;
    falseColMono = false;

    radianceReturn = GLOBAL_RADIANCE;

    minimumLightPathDepth = 0;
    maximumLightPathDepth = 7;
}
