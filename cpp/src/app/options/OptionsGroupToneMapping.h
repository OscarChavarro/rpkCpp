#ifndef COMMAND_LINE_TONE_MAPPING_OPTIONS_GROUP__
#define COMMAND_LINE_TONE_MAPPING_OPTIONS_GROUP__

#include "common/color/ColorRgb.h"
#include "common/linealAlgebra/Vector3D.h"
#include "tonemap/ToneMappingContext.h"

class OptionsGroupToneMapping final {
  public:
    static void toneMapParseOptions(
        int *argc,
        char **argv,
        char *toneMapNameOut,
        ToneMappingContext &toneMapOptionsContext);

  private:
    static constexpr int TONE_MAPPING_METHODS_STRING_LENGTH = 1000;

    static char toneMappingMethodsString[TONE_MAPPING_METHODS_STRING_LENGTH];
    static char *toneMapName;
    static ToneMappingContext *toneMapOptions;

    static void makeToneMappingMethodsString();
    static void toneMappingMethodOption(char *&name);
    static void brightnessAdjustOption(float &value);
    static void redChromaOption(Vector3D &value);
    static void greenChromaOption(Vector3D &value);
    static void blueChromaOption(Vector3D &value);
    static void whiteChromaOption(Vector3D &value);
    static void toneMappingCommandLineOptionDescAdaptMethodOption(char *&value);
    static void gammaOption(float &value);
    static bool parseColor3(int argc, char **argv, ColorRgb &value);
    static bool parseCieXy(int argc, char **argv, Vector3D &value);
};

#endif
