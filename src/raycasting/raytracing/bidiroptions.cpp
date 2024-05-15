#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

/**
Implements option handling for bidirectional path tracing
*/

#include "common/options.h"
#include "raycasting/raytracing/bidiroptions.h"

/**
User interface links
*/
void
biDirectionalPathDefaults() {
    GLOBAL_rayTracing_biDirectionalPath.basecfg.samplesPerPixel = 1;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.progressiveTracing = true;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.minimumPathDepth = 2;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumPathDepth = 7;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumEyePathDepth = 7;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumLightPathDepth = 7;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.sampleImportantLights = true;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.useSpars = false;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.doLe = true;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.doLD = false;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.doLI = false;

    // Weighted not in UI
    GLOBAL_rayTracing_biDirectionalPath.basecfg.doWeighted = false;

    snprintf(GLOBAL_rayTracing_biDirectionalPath.basecfg.leRegExp, MAX_REGEXP_SIZE, "(LX)(X)*(EX)");
    snprintf(GLOBAL_rayTracing_biDirectionalPath.basecfg.ldRegExp, MAX_REGEXP_SIZE, "(LX)(G|S)(X)*(EX),(LX)(EX)");
    snprintf(GLOBAL_rayTracing_biDirectionalPath.basecfg.liRegExp, MAX_REGEXP_SIZE, "(LX)(G|S)(X)*(EX),(LX)(EX)");
    snprintf(GLOBAL_rayTracing_biDirectionalPath.basecfg.wleRegExp, MAX_REGEXP_SIZE, "(LX)(DR)(X)*(EX)");
    snprintf(GLOBAL_rayTracing_biDirectionalPath.basecfg.wldRegExp, MAX_REGEXP_SIZE, "(LX)(X)*(EX)");

    // Not in UI yet
    GLOBAL_rayTracing_biDirectionalPath.saveSubsequentImages = false;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.eliminateSpikes = false;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.doDensityEstimation = false;
    GLOBAL_rayTracing_biDirectionalPath.baseFilename[0] = '\0';
}

#endif
