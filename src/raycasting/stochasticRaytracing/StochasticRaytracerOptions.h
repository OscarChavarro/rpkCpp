/**
Options and global state vars for stochastic raytracing
*/

#ifndef __STOCHASTIC_RAYTRACER_OPTIONS__
#define __STOCHASTIC_RAYTRACER_OPTIONS__

#include "render/ScreenBuffer.h"

/*** Typedefs & enums ***/

enum RTSSAMPLING_MODE {
    BRDF_SAMPLING,
    CLASSICAL_SAMPLING,
    PHOTON_MAP_SAMPLING
};

enum RTSLIGHT_MODE {
    POWER_LIGHTS,
    IMPORTANT_LIGHTS,
    ALL_LIGHTS
};

enum RTSRAD_MODE {
    STORED_NONE,
    STORED_DIRECT,
    STORED_INDIRECT,
    STORED_PHOTON_MAP
};

/*** The global state structure ***/

class RTStochastic_State {
  public:
    // Pixel sampling
    int samplesPerPixel;
    int progressiveTracing;

    int doFrameCoherent;
    int doCorrelatedSampling;
    long int baseSeed;

    // Stored radiance handling
    RTSRAD_MODE radMode;

    /* Direct Light sampling */
    int nextEvent;
    int nextEventSamples;
    RTSLIGHT_MODE lightMode;

    /* Background */
    int backgroundDirect;
    int backgroundIndirect;
    int backgroundSampling;

    /* Scattering */
    int scatterSamples;
    int differentFirstDG;
    int firstDGSamples;
    int separateSpecular;
    RTSSAMPLING_MODE reflectionSampling;

    int minPathDepth;
    int maxPathDepth;

    ScreenBuffer *lastscreen;
};


/*** The global state ***/

extern RTStochastic_State GLOBAL_raytracing_state;


/*** Function prototypers ***/

void stochasticRayTracerDefaults();

void RTStochasticParseOptions(int *argc, char **argv);

#endif
