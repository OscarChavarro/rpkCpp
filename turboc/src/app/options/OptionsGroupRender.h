#ifndef CMMND_LINE_RNDR_OPTNS_GRP
#define CMMND_LINE_RNDR_OPTNS_GRP

#include "material/RendererConfiguration.h"

class OptionsGroupRender{ public:
    static void renderParseOptions( int *argc, char **argv, RenderOptions *renderOptions);

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
