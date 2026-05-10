#ifndef __STOCHASTIC_RAYTRACER_CALLBACK_DATA__
#define __STOCHASTIC_RAYTRACER_CALLBACK_DATA__

#include "material/RendererConfiguration.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracingConfiguration.h"
#include "scene/RadianceMethod.h"

class StochasticRaytracerCallbackData {
  public:
    StochasticRaytracingConfiguration *config;
    RadianceMethod *radianceMethod;
    RendererConfiguration *renderOptions;
};

#endif
