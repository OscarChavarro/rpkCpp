#ifndef STCHS_RYTRC_CLLBC_DATA
#define STCHS_RYTRC_CLLBC_DATA

#include "common/RenderOptions.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracingConfiguration.h"
#include "scene/RadianceMethod.h"

class StochasticRaytracerCallbackData {
  public:
    StochRaytrConfig *config;
    RadianceMethod *radianceMethod;
    RenderOptions *renderOptions;
};

#endif
