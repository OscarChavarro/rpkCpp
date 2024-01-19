/* bidiroptions.H: encapsulates option handling for bidirectional
   path tracing */

#ifndef _BIDIROPTIONS_H_
#define _BIDIROPTIONS_H_

#include "raycasting/common/ScreenBuffer.h"

/* BP_BASECONFIG contains basic config options used 
   throughout the complete BPT code. typically two
   instances are used. One persistent, containing current
   GUI options and one non persistant copy when rendering an 
   image 
*/

#define MAX_REGEXP_SIZE 100

class BP_BASECONFIG {
  public:
    /* UI configurable */

    /* Path depth config */
    int maximumEyePathDepth;
    int maximumLightPathDepth;
    int maximumPathDepth;
    int minimumPathDepth;

    /* Sampling details */
    int samplesPerPixel;
    long totalSamples;
    int sampleImportantLights;
    int progressiveTracing;
    int eliminateSpikes;

    /* SPaR details (usage of reg exps) */

    int useSpars;
    int doLe;
    int doLD;
    int doLI;

    char leRegExp[MAX_REGEXP_SIZE];
    char ldRegExp[MAX_REGEXP_SIZE];
    char liRegExp[MAX_REGEXP_SIZE];

    int doWeighted; /* Use weighted multipass method for
		     a part of le and ld transport */
    int doWLe;
    int doWLD;
    char wleRegExp[MAX_REGEXP_SIZE];
    char wldRegExp[MAX_REGEXP_SIZE];

    /* These values are not yet in the presets or UI !! */
    int doDensityEstimation;
};


/* BIDIRPATH_STATE : typedef of the persistent BPT state */
class BIDIRPATH_STATE {
  public:
    BP_BASECONFIG basecfg;
    ScreenBuffer *lastscreen;

    char baseFilename[255];
    int saveSubsequentImages;
};

/*** Global state of bidirectional path tracing ***/
extern BIDIRPATH_STATE bidir;


/*** Function prototypes ***/

void biDirectionalPathDefaults();

void biDirectionalPathParseOptions(int *argc, char **argv);

void BidirPathPrintOptions(FILE *fp);

#endif /* _BIDIROPTIONS_H_ */
