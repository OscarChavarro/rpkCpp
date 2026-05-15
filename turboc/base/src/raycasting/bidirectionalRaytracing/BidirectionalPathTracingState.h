#ifndef BDRCT_PATH_TRCNG_STT
#define BDRCT_PATH_TRCNG_STT


#include "common/VSDK.h"
#include <string.h>

#include "raycasting/bidirectionalRaytracing/BidirectionalPathRaytracerConfig.h"

/**
Persistent BPT state
*/
class BidirectionalPathTracingState {
  public:
    BidirectionalPathTracingState():
        baseConfig(),
        lastScreen(NULL),
        saveSubsequentImages(false)
    {
        baseConfig.samplesPerPixel = 1;
        baseConfig.progressiveTracing = true;
        baseConfig.minimumPathDepth = 2;
        baseConfig.maximumPathDepth = 7;
        baseConfig.maximumEyePathDepth = 7;
        baseConfig.maximumLightPathDepth = 7;
        baseConfig.sampleImportantLights = true;
        baseConfig.useSpars = false;
        baseConfig.doLe = true;
        baseConfig.doLD = false;
        baseConfig.doLI = false;

        baseConfig.doWeighted = false;

        strcpy(baseConfig.leRegExp, "(LX)(X)*(EX)");
        strcpy(baseConfig.ldRegExp, "(LX)(G|S)(X)*(EX),(LX)(EX)");
        strcpy(baseConfig.liRegExp, "(LX)(G|S)(X)*(EX),(LX)(EX)");
        strcpy(baseConfig.wleRegExp, "(LX)(DR)(X)*(EX)");
        strcpy(baseConfig.wldRegExp, "(LX)(X)*(EX)");

        baseConfig.eliminateSpikes = false;
        baseConfig.doDensityEstimation = false;
        baseFilename[0] = '\0';
    }

    BidirPathRaytrcCnfg baseConfig;
    ScreenBuffer *lastScreen;
    char baseFilename[255];
    int saveSubsequentImages;
};

#endif
