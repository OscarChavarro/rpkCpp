#ifndef __PHOTON_MAP_STATE__
#define __PHOTON_MAP_STATE__

#include "vsdk/toolkit/render/ScreenBuffer.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMapDCAcceptPDFType.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMapDensityControlOption.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMapImportanceOption.h"
#include "vsdk/toolkit/raycasting/photonMap/RadiosityReturnOption.h"

class PhotonMapState {
  public:
    static constexpr int MAXIMUM_RECON_PHOTONS = 400;

    int doGlobalMap;
    long gPathsPerIteration;
    int precomputeGIrradiance;
    int doCausticMap;
    long cPathsPerIteration;
    int renderImage;
    int reconGPhotons;
    int reconCPhotons;
    int reconIPhotons;
    int distribPhotons;
    int balanceKDTree;
    int usePhotonMapSampler;
    PhotonMapDensityControlOption densityControl;
    PhotonMapImportanceOption importanceOption;
    PhotonMapDCAcceptPDFType acceptPdfType;
    double constantRD;
    double minimumImpRD;
    int doImportanceMap;
    long iPathsPerIteration;
    double cImpScale;
    double gImpScale;
    double gThreshold;
    double falseColMax;
    int falseColLog;
    int falseColMono;
    RadiosityReturnOption radianceReturn;
    int minimumLightPathDepth;
    int maximumLightPathDepth;
    int iterationNumber;
    int gIterationNumber;
    int cIterationNumber;
    int i_iteration_nr;
    long totalCPaths;
    long totalGPaths;
    long totalIPaths;
    int runStopNumber; // Number of 'external iterations'. This is
		     // different from currentIteration only when
		     // statistics are gathered
    double cpuSecs; // For counting computing times
    long long lastClock;

    PhotonMapState();
    virtual ~PhotonMapState();
    virtual void setDefaults();
};

#endif
