// pmapoptions.H : class for Photon map options

#ifndef _PMAPOPTIONS_H_
#define _PMAPOPTIONS_H_

#include "shared/options.h"  // For parsing options
#include "material/color.h"
#include <ctime>

#include "raycasting/common/ScreenBuffer.h"


/**** Used constants ****/

const int MAXRECONPHOTONS = 400;

enum PMAP_TYPE {
    GLOBAL_MAP,
    CAUSTIC_MAP,
    IMPORTANCE_MAP
};

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

/**** Photon map options ****/

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

    /*
      int minimumEyePathDepth;
      int maximumEyePathDepth;
    */

    int minimumLightPathDepth;
    int maximumLightPathDepth;
    int maxCombinedLength;


    // Other (changing) state options

    int iteration_nr;
    int g_iteration_nr;
    int c_iteration_nr;
    int i_iteration_nr;

    long total_cpaths, total_gpaths, total_ipaths;

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


extern CPmapState pmapstate;

extern void photonMapParseOptions(int *argc, char **argv);

extern void photonMapPrintOptions(FILE *fp);


#endif /* _PMAPOPTIONS_H_ */
