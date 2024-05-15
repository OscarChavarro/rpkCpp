/**
Options and global state vars for stochastic raytracing
*/

#ifndef __STOCHASTIC_RAYTRACER_OPTIONS__
#define __STOCHASTIC_RAYTRACER_OPTIONS__

#include "render/ScreenBuffer.h"

/*** Typedefs & enums ***/

enum RayTracingSamplingMode {
    BRDF_SAMPLING,
    CLASSICAL_SAMPLING,
    PHOTON_MAP_SAMPLING
};

enum RayTracingLightMode {
    POWER_LIGHTS,
    IMPORTANT_LIGHTS,
    ALL_LIGHTS
};

enum RayTracingRadMode {
    STORED_NONE,
    STORED_DIRECT,
    STORED_INDIRECT,
    STORED_PHOTON_MAP
};

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

void stochasticRayTracerDefaults();

#endif
