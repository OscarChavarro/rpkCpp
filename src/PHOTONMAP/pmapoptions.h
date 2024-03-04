#ifndef __PHOTON_MAP_OPTIONS__
#define __PHOTON_MAP_OPTIONS__

#include <ctime>

#include "common/color.h"
#include "render/ScreenBuffer.h"

const int MAXIMUM_RECON_PHOTONS = 400;

enum RADRETURN_OPTION {
    GLOBAL_DENSITY,
    CAUSTIC_DENSITY,
    REC_CDENSITY,
    REC_GDENSITY,
    IMPORTANCE_CDENSITY,
    IMPORTANCE_GDENSITY,
    GLOBAL_RADIANCE,
    CAUSTIC_RADIANCE
};

enum DENSITY_CONTROL_OPTION {
    NO_DENSITY_CONTROL,
    CONSTANT_RD,
    IMPORTANCE_RD
};

enum IMPORTANCE_OPTION {
    USE_IMPORTANCE = 0
};

enum DC_ACCEPTPDFTYPE {
    STEP,
    TRANSCOSINE
};

// Photon map options

class CPmapState
{
  public:
    int doGlobalMap;
    long gpaths_per_iteration;
    int precomputeGIrradiance;

    int doCausticMap;
    long cpaths_per_iteration;

    int renderImage;

    int reconGPhotons;
    int reconCPhotons;
    int reconIPhotons;
    int distribPhotons;

    int doStats;

    int balanceKDTree;

    int usePhotonMapSampler;

    DENSITY_CONTROL_OPTION densityControl;
    IMPORTANCE_OPTION importanceOption;

    DC_ACCEPTPDFTYPE acceptPdfType;

    float constantRD;

    float minimumImpRD;
    int doImportanceMap;
    long ipaths_per_iteration;

    float cImpScale;
    float gImpScale;

    int cornerScatter;
    float gThreshold;

    float falseColMax;
    int falseColLog;
    int falseColMono;

    RADRETURN_OPTION radianceReturn;
    int returnCImportance;
    int minimumLightPathDepth;
    int maximumLightPathDepth;
    int maxCombinedLength;

    // Other (changing) state options
    int iteration_nr;
    int g_iteration_nr;
    int c_iteration_nr;
    int i_iteration_nr;

    long total_cpaths;
    long total_gpaths;
    long total_ipaths;

    int runstop_nr; /* Number of 'external iterations'. This is
		     different from currentIteration only when
		     statistics are gathered. */
    long total_rays;

    float cpu_secs;    /* for counting computing times */
    clock_t lastclock;    /* " */
    int wake_up;        /* check this value at safe positions during the
			 * computations in order to react on user input
			 * if there is any (call CheckForEvents()) */

    ScreenBuffer *rcScreen;

    // Methods

    CPmapState();

    virtual void Defaults();
};

extern CPmapState GLOBAL_photonMap_state;

extern void photonMapParseOptions(int *argc, char **argv);

#endif
