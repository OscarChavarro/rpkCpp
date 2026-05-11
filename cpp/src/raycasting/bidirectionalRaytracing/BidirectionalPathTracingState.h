#ifndef BIDIRECTIONAL_PATH_TRACING_STATE__
#define BIDIRECTIONAL_PATH_TRACING_STATE__

#include <cstring>

#include "raycasting/bidirectionalRaytracing/BidirectionalPathRaytracerConfig.h"

/**
Persistent BPT state
*/
class BidirectionalPathTracingState {
  public:
    BidirectionalPathTracingState():
        baseConfig(),
        lastScreen(nullptr),
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

        std::strcpy(baseConfig.leRegExp, "(LX)(X)*(EX)");
        std::strcpy(baseConfig.ldRegExp, "(LX)(G|S)(X)*(EX),(LX)(EX)");
        std::strcpy(baseConfig.liRegExp, "(LX)(G|S)(X)*(EX),(LX)(EX)");
        std::strcpy(baseConfig.wleRegExp, "(LX)(DR)(X)*(EX)");
        std::strcpy(baseConfig.wldRegExp, "(LX)(X)*(EX)");

        baseConfig.eliminateSpikes = false;
        baseConfig.doDensityEstimation = false;
        baseFilename[0] = '\0';
    }

    BidirectionalPathRaytracerConfig baseConfig;
    ScreenBuffer *lastScreen;
    char baseFilename[255];
    int saveSubsequentImages;
};

#endif
