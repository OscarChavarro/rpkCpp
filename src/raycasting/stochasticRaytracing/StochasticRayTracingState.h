/**
Options and global state vars for stochastic raytracing
*/

#ifndef __STOCHASTIC_RAYTRACER_OPTIONS__
#define __STOCHASTIC_RAYTRACER_OPTIONS__

#include "render/ScreenBuffer.h"
#include "raycasting/stochasticRaytracing/RayTracingSamplingMode.h"
#include "raycasting/stochasticRaytracing/RayTracingLightMode.h"
#include "raycasting/stochasticRaytracing/RayTracingRadMode.h"

class StochasticRayTracingState {
  public:
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

extern StochasticRayTracingState GLOBAL_raytracing_state;

#endif
