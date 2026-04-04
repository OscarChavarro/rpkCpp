#ifndef __COMMAND_LINE_RENDER_OPTIONS_GROUP__
#define __COMMAND_LINE_RENDER_OPTIONS_GROUP__

#include "common/RenderOptions.h"

class OptionsGroupRender final {
  public:
    static void renderParseOptions(
        int *argc,
        char **argv,
        RenderOptions *renderOptions);

  private:
    static int trueValue;
    static RenderOptions renderOptionsState;
    static ColorRgb outlineColor;

    static void flatOption(int &value);
    static void noCullingOption(int &value);
    static void outlinesOption(int &value);
    static void traceOption(int &value);
    static bool parseColor3(int argc, char **argv, ColorRgb &value);
};

#endif
