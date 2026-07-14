#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathTracingConfiguration.h"

BidirectionalPathTracingConfiguration::BidirectionalPathTracingConfiguration():
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
