#ifndef __OPTIONS_GROUP_RAY_MATTER__
#define __OPTIONS_GROUP_RAY_MATTER__

#include "raycasting/common/RayTracer.h"
#include "raycasting/simple/RayMatterState.h"
#include "app/options/EnumDesc.h"

class OptionsGroupRayMatter{ public:
    static void rayMattingParseOptions( int *argc, char **argv, RayMatterState &rayMatterState);

  private:
    template<typename T>
    class EnumBinding{ public:
        T *target;
        const EnumDesc *values;
    };

    static EnumDesc rayMatterPixelFilterValues[];
    static bool parseRayMatterFilterBinding(int argc, char **argv, EnumBinding<RayMatterFilterType> &binding);

    template<typename T>
    static bool parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding);
};

#endif
