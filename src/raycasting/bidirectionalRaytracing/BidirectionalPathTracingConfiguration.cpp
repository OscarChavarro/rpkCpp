#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingConfiguration.h"

BidirectionalPathTracingConfiguration::BidirectionalPathTracingConfiguration():
    baseConfig(),
    eyeConfig(),
    lightConfig(),
    screen(),
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
