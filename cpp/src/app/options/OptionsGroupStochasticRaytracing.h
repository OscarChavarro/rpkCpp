#ifndef OPTIONS_GROUP_STOCHASTIC_RAYTRACING__
#define OPTIONS_GROUP_STOCHASTIC_RAYTRACING__

#include "app/options/EnumDesc.h"
#include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"

class OptionsGroupStochasticRaytracing final {
  public:
    static void parse(
        int *argc,
        char **argv,
        StochasticRayTracingState &stochasticRayTracingState);

  private:
    template<typename T>
    class EnumBinding {
      public:
        T *target;
        const EnumDesc *values;
    };

    static EnumDesc rayTracingRadianceModeValues[];
    static EnumDesc rayTracingLightModeValues[];
    static EnumDesc rayTracingSamplingModeValues[];

    template<typename T>
    static bool parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding);
    static void setIntTrue(int &value);
    static void setIntFalse(int &value);
};

#endif
