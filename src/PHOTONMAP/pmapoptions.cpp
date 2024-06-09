#include "PHOTONMAP/pmapoptions.h"

PhotonMapState GLOBAL_photonMap_state;

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
    setDefaults();
}

PhotonMapState::~PhotonMapState() {
}

void
PhotonMapState::setDefaults() {
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

    radianceReturn = RadiosityReturnOption::GLOBAL_RADIANCE;

    minimumLightPathDepth = 0;
    maximumLightPathDepth = 7;
}
