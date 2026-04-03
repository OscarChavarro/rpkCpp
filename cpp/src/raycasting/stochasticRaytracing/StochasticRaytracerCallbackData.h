#ifndef __STOCHASTIC_RAYTRACER_CALLBACK_DATA__
#define __STOCHASTIC_RAYTRACER_CALLBACK_DATA__

#include "common/RenderOptions.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracingConfiguration.h"
#include "scene/RadianceMethod.h"

class StochasticRaytracerCallbackData {
  public:
    StochasticRaytracingConfiguration *config;
    RadianceMethod *radianceMethod;
    RenderOptions *renderOptions;
};

#endif
