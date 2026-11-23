#ifndef STCHS_RYTRC_CLLBC_DATA
#define STCHS_RYTRC_CLLBC_DATA

#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRaytracingConfiguration.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"

class StochasticRaytracerCallbackData {
  public:
    StochRaytrConfig *config;
    RadianceMethod *radianceMethod;
    RenderOptions *renderOptions;
    StochasticRaytracerCallbackData(StochRaytrConfig *inConfig, RadianceMethod *inRadianceMethod,
                                    RenderOptions *inRenderOptions):
        config(inConfig), radianceMethod(inRadianceMethod), renderOptions(inRenderOptions) {}
};

#endif
