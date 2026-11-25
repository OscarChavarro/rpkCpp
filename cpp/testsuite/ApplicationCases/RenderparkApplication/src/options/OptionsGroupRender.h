#ifndef __OPTIONS_GROUP_RENDER__
#define __OPTIONS_GROUP_RENDER__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"

class OptionsGroupRender final {
  public:
    static void renderParseOptions(
        int *argc,
        char **argv,
        RendererConfiguration *renderOptions);

  private:
    static int trueValue;
    static RendererConfiguration renderOptionsState;
    static ColorRgb outlineColor;

    static void flatOption(int &value);
    static void noCullingOption(int &value);
    static void outlinesOption(int &value);
    static void traceOption(int &value);
    static bool parseColor3(int argc, char **argv, ColorRgb &value);
};

#endif
