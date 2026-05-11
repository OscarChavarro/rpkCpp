/**
Options and runtime state for stochastic raytracing.
*/

#ifndef STOCHASTIC_RAYTRACER_OPTIONS__
#define STOCHASTIC_RAYTRACER_OPTIONS__

#include "vsdk/toolkit/render/ScreenBuffer.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/RayTracingLightMode.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/RayTracingRadMode.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/RayTracingSamplingMode.h"

class StochasticRayTracingState {
  public:
    StochasticRayTracingState():
        samplesPerPixel(1),
        progressiveTracing(true),
        doFrameCoherent(false),
        doCorrelatedSampling(false),
        baseSeed(0xFE062134),
        radMode(RayTracingRadMode::STORED_NONE),
        nextEvent(true),
        nextEventSamples(1),
        lightMode(RayTracingLightMode::ALL_LIGHTS),
        backgroundDirect(false),
        backgroundIndirect(true),
        backgroundSampling(false),
        scatterSamples(1),
        differentFirstDG(false),
        firstDGSamples(36),
        separateSpecular(false),
        reflectionSampling(RayTracingSamplingMode::BRDF_SAMPLING),
        minPathDepth(5),
        maxPathDepth(7),
        lastScreen(nullptr)
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
