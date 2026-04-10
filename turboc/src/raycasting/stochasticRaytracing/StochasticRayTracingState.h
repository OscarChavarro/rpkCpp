/**
Options and runtime state for stochastic raytracing.
*/

#ifndef STCHS_RYTRC_OPTNS
#define STCHS_RYTRC_OPTNS

#include "render/ScreenBuffer.h"
#include "raycasting/stochasticRaytracing/RayTracingLightMode.h"
#include "raycasting/stochasticRaytracing/RayTracingRadMode.h"
#include "raycasting/stochasticRaytracing/RayTracingSamplingMode.h"

class StochasticRayTracingState {
  public:
    StochasticRayTracingState():
        samplesPerPixel(1),
        progressiveTracing(true),
        doFrameCoherent(false),
        doCorrelatedSampling(false),
        baseSeed(0xFE062134),
        radMode(STORED_NONE),
        nextEvent(true),
        nextEventSamples(1),
        lightMode(ALL_LIGHTS),
        backgroundDirect(false),
        backgroundIndirect(true),
        backgroundSampling(false),
        scatterSamples(1),
        differentFirstDG(false),
        firstDGSamples(36),
        separateSpecular(false),
        reflectionSampling(BRDF_SAMPLING),
        minPathDepth(5),
        maxPathDepth(7),
        lastScreen(NULL)
    {
    }

    // Pixel sampling
    int samplesPerPixel;
    int progressiveTracing;

    int doFrameCoherent;
    int doCorrelatedSampling;
    long int baseSeed;

    // Stored radiance handling
    RayTracingRadMode radMode;

    // Direct Light sampling
    int nextEvent;
    int nextEventSamples;
    RayTracingLightMode lightMode;

    // Background
    int backgroundDirect;
    int backgroundIndirect;
    int backgroundSampling;

    // Scattering
    int scatterSamples;
    int differentFirstDG;
    int firstDGSamples;
    int separateSpecular;
    RayTracingSamplingMode reflectionSampling;

    int minPathDepth;
    int maxPathDepth;

    ScreenBuffer *lastScreen;
};

#endif
