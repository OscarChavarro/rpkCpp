#ifndef BIDIRECTIONAL_PATH_TRACING_CONFIGURATION__
#define BIDIRECTIONAL_PATH_TRACING_CONFIGURATION__

#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "vsdk/toolkit/render/ScreenBuffer.h"
#include "vsdk/toolkit/raycasting/raytracing/SamplerConfig.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/DensityBuffer.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/Kernel2D.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathRaytracerConfig.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/Spar.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

/**
Bidirectional path tracing configuration structure.
non persistently used each time an image is rendered
*/
class BidirectionalPathTracingConfiguration {
  public:
    BidirectionalPathRaytracerConfig *baseConfig;

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

    BidirectionalPathTracingConfiguration();
};
#endif

#endif
