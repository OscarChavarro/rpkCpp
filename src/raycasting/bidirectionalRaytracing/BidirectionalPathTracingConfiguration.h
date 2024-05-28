#ifndef __BIDIRECTIONAL_PATH_TRACING_CONFIGURATION__
#define __BIDIRECTIONAL_PATH_TRACING_CONFIGURATION__

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "render/ScreenBuffer.h"
#include "raycasting/raytracing/samplertools.h"
#include "raycasting/bidirectionalRaytracing/DensityBuffer.h"
#include "raycasting/bidirectionalRaytracing/densitykernel.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathRaytracerConfig.h"
#include "raycasting/bidirectionalRaytracing/Spar.h"

/**
Bidirectional path tracing configuration structure.
non persistently used each time an image is rendered
*/
class BidirectionalPathTracingConfiguration {
  public:
    BidirectionalPathRaytracerConfig *baseConfig;

    // Configuration for tracing the paths
    CSamplerConfig eyeConfig;
    CSamplerConfig lightConfig;

    // Internal vars
    ScreenBuffer *screen;
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
    CKernel2D kernel;
    int scaleSamples;

    BidirectionalPathTracingConfiguration();
};
#endif

#endif
