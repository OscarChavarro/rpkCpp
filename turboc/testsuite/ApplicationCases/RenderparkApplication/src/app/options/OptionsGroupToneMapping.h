#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __OPTIONS_GROUP_TONE_MAPPING__
#define __OPTIONS_GROUP_TONE_MAPPING__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

class OptionsGroupToneMapping{ public:
    static void toneMapParseOptions( int *argc, char **argv, char *toneMapNameOut, ToneMappingContext &toneMapOptionsContext);

  private:
    #define TONE_MAP_MTHS_STR_LEN 1000

    static char toneMappingMethodsString[TONE_MAP_MTHS_STR_LEN];
    static char *toneMapName;
    static ToneMappingContext *toneMapOptions;
    static double gammaValue;

    static void makeToneMappingMethodsString();
    static void toneMappingMethodOption(char *&name);
    static void brightnessAdjustOption(double &value);
    static void redChromaOption(Vector3Dd &value);
    static void greenChromaOption(Vector3Dd &value);
    static void blueChromaOption(Vector3Dd &value);
    static void whiteChromaOption(Vector3Dd &value);
    static void tonMppCmdLinOptDesAdaMetOpt(char *&value);
    static void gammaOption(double &value);
    static bool parseColor3(int argc, char **argv, ColorRgb &value);
    static bool parseCieXy(int argc, char **argv, Vector3Dd &value);
};

#endif
