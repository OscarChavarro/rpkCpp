#ifndef OPTNS_GRP_STCHS_RYTRC
#define OPTNS_GRP_STCHS_RYTRC

#include "app/options/EnumDesc.h"
#include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"

class OptsGrpStochRaytr{ public:
    static void parse( int *argc, char **argv, StochasticRayTracingState &stochasticRayTracingState);

  private:
    template<typename T>
    class EnumBinding{ public:
        T *target;
        const EnumDesc *values;
    };

    static EnumDesc rayTracingRadianceModeValues[];
    static EnumDesc rayTracingLightModeValues[];
    static EnumDesc rayTracingSamplingModeValues[];
    static bool parseRadModeBinding(int argc, char **argv, EnumBinding<RayTracingRadMode> &binding);
    static bool parseLightModeBinding(int argc, char **argv, EnumBinding<RayTracingLightMode> &binding);
    static bool parseSamplingModeBinding(int argc, char **argv, EnumBinding<RayTracingSamplingMode> &binding);

    template<typename T>
    static bool parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding);
    static void setIntTrue(int &value);
    static void setIntFalse(int &value);
};

#endif
