#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED
#include "material/RendererConfiguration.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingConfiguration.h"

BidirPathTraceConfig::BidirPathTraceConfig():
    baseConfig(),
    eyeConfig(),
    lightConfig(),
    screen(),
    toneMapOptions(),
    fluxToRadFactor(),
    nx(),
    ny(),
    pdfLNE(),
    dBuffer(),
    dBuffer2(),
    xSample(),
    ySample(),
    eyePath(),
    lightPath(),
    sparConfig(),
    sparList(),
    deStoreHits(),
    ref(),
    dest(),
    ref2(),
    dest2(),
    kernel(),
    scaleSamples()
{}

#endif
