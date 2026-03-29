#ifndef __STOCHASTIC_RAYTRACER_CALLBACK_DATA__
#define __STOCHASTIC_RAYTRACER_CALLBACK_DATA__

class StochasticRaytracingConfiguration;
class RadianceMethod;
class RenderOptions;

class StochasticRaytracerCallbackData {
  public:
    StochasticRaytracingConfiguration *config;
    RadianceMethod *radianceMethod;
    RenderOptions *renderOptions;
};

#endif
