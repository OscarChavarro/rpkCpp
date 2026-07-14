#ifndef STOCHASTIC_RAYTRACER_CALLBACK_DATA__
#define STOCHASTIC_RAYTRACER_CALLBACK_DATA__

#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRaytracingConfiguration.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"

class StochasticRaytracerCallbackData {
  public:
    StochasticRaytracingConfiguration *config;
    RadianceMethod *radianceMethod;
    RendererConfiguration *renderOptions;
};

#endif
