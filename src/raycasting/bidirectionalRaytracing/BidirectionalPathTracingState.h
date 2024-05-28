#ifndef __BIDIRECTIONAL_PATH_TRACING_STATE__
#define __BIDIRECTIONAL_PATH_TRACING_STATE__

#include "raycasting/bidirectionalRaytracing/BidirectionalPathRaytracerConfig.h"

/**
Persistent BPT state
*/
class BidirectionalPathTracingState {
  public:
    BidirectionalPathRaytracerConfig baseConfig;
    ScreenBuffer *lastScreen;
    char baseFilename[255];
    int saveSubsequentImages;
};

// Global state of bidirectional path tracing
extern BidirectionalPathTracingState GLOBAL_rayTracing_biDirectionalPath;

#endif
