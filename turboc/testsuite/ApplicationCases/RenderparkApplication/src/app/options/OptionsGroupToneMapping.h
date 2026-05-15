#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef CMMND_LINE_TONE_MAP_OPTNS_GRP
#define CMMND_LINE_TONE_MAP_OPTNS_GRP

#include "common/color/ColorRgb.h"
#include "common/linealAlgebra/Vector3D.h"
#include "tonemap/ToneMappingContext.h"

class OptionsGroupToneMapping{ public:
    static void toneMapParseOptions( int *argc, char **argv, char *toneMapNameOut, ToneMappingContext &toneMapOptionsContext);

  private:
    #define TONE_MAP_MTHS_STR_LEN 1000

    static char toneMappingMethodsString[TONE_MAP_MTHS_STR_LEN];
    static char *toneMapName;
    static ToneMappingContext *toneMapOptions;
    static float gammaValue;

    static void makeToneMappingMethodsString();
    static void toneMappingMethodOption(char *&name);
    static void brightnessAdjustOption(float &value);
    static void redChromaOption(Vector3D &value);
    static void greenChromaOption(Vector3D &value);
    static void blueChromaOption(Vector3D &value);
    static void whiteChromaOption(Vector3D &value);
    static void tonMppCmdLinOptDesAdaMetOpt(char *&value);
    static void gammaOption(float &value);
    static bool parseColor3(int argc, char **argv, ColorRgb &value);
    static bool parseCieXy(int argc, char **argv, Vector3D &value);
};

#endif
