#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __PHOTON_MAP_OPTIONS__
#define __PHOTON_MAP_OPTIONS__

#include "render/ScreenBuffer.h"
#include "raycasting/photonMap/PhotonMapDCAcceptPDFType.h"
#include "raycasting/photonMap/PhotonMapDensityControlOption.h"
#include "raycasting/photonMap/PhotonMapImportanceOption.h"
#include "raycasting/photonMap/RadiosityReturnOption.h"

#define PHOTON_MAP_MAXIMUM_RECON_PHOTONS 400

class PhotonMapState {
  public:
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
    float constantRD;
    float minimumImpRD;
    int doImportanceMap;
    long iPathsPerIteration;
    float cImpScale;
    float gImpScale;
    float gThreshold;
    float falseColMax;
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
    float cpuSecs; // For counting computing times
    long lastClock;

    PhotonMapState();
    virtual ~PhotonMapState();
    virtual void setDefaults();
};

#endif
