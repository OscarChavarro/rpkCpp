#ifndef COMMAND_LINE_OPTIONS__
#define COMMAND_LINE_OPTIONS__

#include "vsdk/toolkit/raycasting/common/RayTracer.h"
#include "vsdk/toolkit/raycasting/simple/RayMatterState.h"
#include "options/EnumDesc.h"

class OptionsGroupRayMatter final {
  public:
    static void rayMattingParseOptions(
            int *argc,
            char **argv,
            RayMatterState &rayMatterState);

  private:
    template<typename T>
    class EnumBinding {
      public:
        T *target;
        const EnumDesc *values;
    };

    static EnumDesc rayMatterPixelFilterValues[];

    template<typename T>
    static bool parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding);
};

#endif
