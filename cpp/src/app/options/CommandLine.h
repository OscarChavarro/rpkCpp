#ifndef __COMMAND_LINE_OPTIONS__
#define __COMMAND_LINE_OPTIONS__

#include "raycasting/common/RayTracer.h"
#include "raycasting/simple/RayMatterState.h"
#include "app/options/EnumDesc.h"

class CommandLine final {
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
