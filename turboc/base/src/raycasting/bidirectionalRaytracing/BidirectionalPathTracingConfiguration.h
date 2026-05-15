#ifndef BDRCT_PATH_TRCNG_CNFGR
#define BDRCT_PATH_TRCNG_CNFGR

#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "render/ScreenBuffer.h"
#include "raycasting/raytracing/SamplerConfig.h"
#include "raycasting/bidirectionalRaytracing/DensityBuffer.h"
#include "raycasting/bidirectionalRaytracing/Kernel2D.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathRaytracerConfig.h"
#include "raycasting/bidirectionalRaytracing/Spar.h"
#include "tonemap/ToneMappingContext.h"

/**
Bidirectional path tracing configuration structure.
non persistently used each time an image is rendered
*/
class BidirPathTraceConfig {
  public:
    BidirPathRaytrcCnfg *baseConfig;

    // Configuration for tracing the paths
    SamplerConfig eyeConfig;
    SamplerConfig lightConfig;

    // Internal vars
    ScreenBuffer *screen;
    ToneMappingContext *toneMapOptions;
    double fluxToRadFactor;
    int nx;
    int ny;
    double pdfLNE; // pdf for sampling light point separately

    DensityBuffer *dBuffer;
    DensityBuffer *dBuffer2;
    float xSample;
    float ySample;
    SimpleRaytracingPathNode *eyePath;
    SimpleRaytracingPathNode *lightPath;

    // SPaR configuration
    SparConfig sparConfig;
    SparList *sparList;
    bool deStoreHits;
    ScreenBuffer *ref;
    ScreenBuffer *dest;
    ScreenBuffer *ref2;
    ScreenBuffer *dest2;
    Kernel2D kernel;
    int scaleSamples;

    BidirPathTraceConfig();
};
#endif

#endif
