#ifndef __PHOTON_MAP_OPTIONS__
#define __PHOTON_MAP_OPTIONS__

#include <ctime>

#include "render/ScreenBuffer.h"
#include "PHOTONMAP/RadiosityReturnOption.h"

const int MAXIMUM_RECON_PHOTONS = 400;

enum DENSITY_CONTROL_OPTION {
    NO_DENSITY_CONTROL,
    CONSTANT_RD,
    IMPORTANCE_RD
};

enum IMPORTANCE_OPTION {
    USE_IMPORTANCE = 0
};

enum DC_ACCEPT_PDF_TYPE {
    STEP,
    TRANS_COSINE
};

// Photon map options
class PhotonMapState
{
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
    DENSITY_CONTROL_OPTION densityControl;
    IMPORTANCE_OPTION importanceOption;
    DC_ACCEPT_PDF_TYPE acceptPdfType;
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
    clock_t lastClock;

    PhotonMapState();
    virtual ~PhotonMapState();
    virtual void setDefaults();
};

extern PhotonMapState GLOBAL_photonMap_state;

#endif
