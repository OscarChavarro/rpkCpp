#include <cstdlib>
#include <cstring>

#include "vsdk/toolkit/java/lang/Math.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/color/Cie.h"
#include "vsdk/toolkit/common/commandLineOptions/OptionParser.h"
#include "vsdk/toolkit/common/commandLineOptions/TypedOption.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"
#include "options/OptionsGroupToneMapping.h"

char OptionsGroupToneMapping::toneMappingMethodsString[OptionsGroupToneMapping::TONE_MAPPING_METHODS_STRING_LENGTH];
char *OptionsGroupToneMapping::toneMapName = nullptr;
ToneMappingContext *OptionsGroupToneMapping::toneMapOptions = nullptr;

void
OptionsGroupToneMapping::makeToneMappingMethodsString() {
    strcpy(toneMappingMethodsString,
       "-tonemapping <method>: Set tone mapping method\n"
       "\tmethods: Lightness            Lightness Mapping (default)\n"
       "\t         TumblinRushmeier     Tumblin/Rushmeier's Mapping\n"
       "\t         Ward                 Ward's Mapping\n"
       "\t         RevisedTR            Revised Tumblin/Rushmeier's Mapping\n"
       "\t         Ferwerda             Partial Ferwerda's Mapping");
}

void
OptionsGroupToneMapping::toneMappingMethodOption(char *&name) {
    strcpy(toneMapName, name);
}

void
OptionsGroupToneMapping::brightnessAdjustOption(float & /*value*/) {
    if ( toneMapOptions == nullptr ) {
        Logger::fatal(-1, "CommandLineToneMappingOptionsGroup::brightnessAdjustOption", "ToneMappingContext not set");
    }
    (*toneMapOptions).pow_bright_adjust = static_cast<float>(java::Math::pow(2.0, static_cast<double>((*toneMapOptions).brightness_adjust)));
}

void
OptionsGroupToneMapping::redChromaOption(Vector3D &value) {
    if ( toneMapOptions == nullptr ) {
        Logger::fatal(-1, "CommandLineToneMappingOptionsGroup::redChromaOption", "ToneMappingContext not set");
    }
    (*toneMapOptions).xr = value.x;
    (*toneMapOptions).yr = value.y;
    Cie::computeColorConversionTransforms(
        (*toneMapOptions).xr, (*toneMapOptions).yr,
        (*toneMapOptions).xg, (*toneMapOptions).yg,
        (*toneMapOptions).xb, (*toneMapOptions).yb,
        (*toneMapOptions).xw, (*toneMapOptions).yw);
}

void
OptionsGroupToneMapping::greenChromaOption(Vector3D &value) {
    if ( toneMapOptions == nullptr ) {
        Logger::fatal(-1, "CommandLineToneMappingOptionsGroup::greenChromaOption", "ToneMappingContext not set");
    }
    (*toneMapOptions).xg = value.x;
    (*toneMapOptions).yg = value.y;
    Cie::computeColorConversionTransforms(
        (*toneMapOptions).xr, (*toneMapOptions).yr,
        (*toneMapOptions).xg, (*toneMapOptions).yg,
        (*toneMapOptions).xb, (*toneMapOptions).yb,
        (*toneMapOptions).xw, (*toneMapOptions).yw);
}

void
OptionsGroupToneMapping::blueChromaOption(Vector3D &value) {
    if ( toneMapOptions == nullptr ) {
        Logger::fatal(-1, "CommandLineToneMappingOptionsGroup::blueChromaOption", "ToneMappingContext not set");
    }
    (*toneMapOptions).xb = value.x;
    (*toneMapOptions).yb = value.y;
    Cie::computeColorConversionTransforms(
        (*toneMapOptions).xr, (*toneMapOptions).yr,
        (*toneMapOptions).xg, (*toneMapOptions).yg,
        (*toneMapOptions).xb, (*toneMapOptions).yb,
        (*toneMapOptions).xw, (*toneMapOptions).yw);
}

void
OptionsGroupToneMapping::whiteChromaOption(Vector3D &value) {
    if ( toneMapOptions == nullptr ) {
        Logger::fatal(-1, "CommandLineToneMappingOptionsGroup::whiteChromaOption", "ToneMappingContext not set");
    }
    (*toneMapOptions).xw = value.x;
    (*toneMapOptions).yw = value.y;
    Cie::computeColorConversionTransforms(
        (*toneMapOptions).xr, (*toneMapOptions).yr,
        (*toneMapOptions).xg, (*toneMapOptions).yg,
        (*toneMapOptions).xb, (*toneMapOptions).yb,
        (*toneMapOptions).xw, (*toneMapOptions).yw);
}

void
OptionsGroupToneMapping::toneMappingCommandLineOptionDescAdaptMethodOption(char *&name) {
    if ( toneMapOptions == nullptr ) {
        Logger::fatal(-1, "CommandLineToneMappingOptionsGroup::toneMappingCommandLineOptionDescAdaptMethodOption", "ToneMappingContext not set");
    }
    if ( strncasecmp(name, "average", 2) == 0 ) {
        (*toneMapOptions).staticAdaptationMethod = ToneMapAdaptationMethod::TMA_AVERAGE;
    } else if ( strncasecmp(name, "median", 2) == 0 ) {
        (*toneMapOptions).staticAdaptationMethod = ToneMapAdaptationMethod::TMA_MEDIAN;
    } else {
        Logger::error(nullptr, "Invalid adaptation estimate method '%s'", name);
    }
}

void
OptionsGroupToneMapping::gammaOption(double &gam) {
    if ( toneMapOptions == nullptr ) {
        Logger::fatal(-1, "CommandLineToneMappingOptionsGroup::gammaOption", "ToneMappingContext not set");
    }
    (*toneMapOptions).gamma = ColorRgb(gam, gam, gam);
}

void
OptionsGroupToneMapping::toneMapParseOptions(
        int *argc,
        char **argv,
        char *toneMapNameOut,
        ToneMappingContext &toneMapOptionsContext)
{
    char *toneMapMethodName = nullptr;
    char *adaptMethodName = nullptr;
    Vector3D redChromaticityValue(0.0, 0.0, 0.0);
    Vector3D greenChromaticityValue(0.0, 0.0, 0.0);
    Vector3D blueChromaticityValue(0.0, 0.0, 0.0);
    Vector3D whiteChromaticityValue(0.0, 0.0, 0.0);
    double gammaScalar = toneMapOptionsContext.gamma.getR();
    TypedOption<char *> toneMappingOpt = {"-tonemapping", &toneMapMethodName, 1, OptionsGroupToneMapping::toneMappingMethodOption, nullptr};
    TypedOption<float> brightnessAdjustOpt = {"-brightness-adjust", &toneMapOptionsContext.brightness_adjust, 1, OptionsGroupToneMapping::brightnessAdjustOption, nullptr};
    TypedOption<char *> adaptOpt = {"-adapt", &adaptMethodName, 1, OptionsGroupToneMapping::toneMappingCommandLineOptionDescAdaptMethodOption, nullptr};
    TypedOption<float> lwaOpt = {"-lwa", &toneMapOptionsContext.realWorldAdaptionLuminance, 1, nullptr, nullptr};
    TypedOption<float> ldmaxOpt = {"-ldmax", &toneMapOptionsContext.maximumDisplayLuminance, 1, nullptr, nullptr};
    TypedOption<float> cmaxOpt = {"-cmax", &toneMapOptionsContext.maximumDisplayContrast, 1, nullptr, nullptr};
    TypedOption<double> gammaOpt = {"-gamma", &gammaScalar, 1, OptionsGroupToneMapping::gammaOption, nullptr};
    TypedOption<ColorRgb> rgbGammaOpt = {"-rgbgamma", &toneMapOptionsContext.gamma, 3, nullptr, OptionsGroupToneMapping::parseColor3};
    TypedOption<Vector3D> redOpt = {"-red", &redChromaticityValue, 2, OptionsGroupToneMapping::redChromaOption, OptionsGroupToneMapping::parseCieXy};
    TypedOption<Vector3D> greenOpt = {"-green", &greenChromaticityValue, 2, OptionsGroupToneMapping::greenChromaOption, OptionsGroupToneMapping::parseCieXy};
    TypedOption<Vector3D> blueOpt = {"-blue", &blueChromaticityValue, 2, OptionsGroupToneMapping::blueChromaOption, OptionsGroupToneMapping::parseCieXy};
    TypedOption<Vector3D> whiteOpt = {"-white", &whiteChromaticityValue, 2, OptionsGroupToneMapping::whiteChromaOption, OptionsGroupToneMapping::parseCieXy};
    OptionBase toneMappingOptions[] = {
        REGISTER_OPTION(char *, toneMappingOpt, 4),
        REGISTER_OPTION(float, brightnessAdjustOpt, 4),
        REGISTER_OPTION(char *, adaptOpt, 5),
        REGISTER_OPTION(float, lwaOpt, 3),
        REGISTER_OPTION(float, ldmaxOpt, 5),
        REGISTER_OPTION(float, cmaxOpt, 4),
        REGISTER_OPTION(double, gammaOpt, 4),
        REGISTER_OPTION(ColorRgb, rgbGammaOpt, 4),
        REGISTER_OPTION(Vector3D, redOpt, 4),
        REGISTER_OPTION(Vector3D, greenOpt, 4),
        REGISTER_OPTION(Vector3D, blueOpt, 4),
        REGISTER_OPTION(Vector3D, whiteOpt, 4)
    };

    OptionsGroupToneMapping::toneMapName = toneMapNameOut;
    OptionsGroupToneMapping::toneMapOptions = &toneMapOptionsContext;
    OptionsGroupToneMapping::makeToneMappingMethodsString();
    OptionGroup toneMappingGroups[] = {
        OptionGroup("toneMapping", toneMappingOptions, 12)
    };
    OptionParser<OptionBase>::parse(argc, argv, toneMappingGroups, 1);
    ToneMap::recomputeGammaTables(toneMapOptionsContext, (*OptionsGroupToneMapping::toneMapOptions).gamma);
    OptionsGroupToneMapping::toneMapOptions = nullptr;
    OptionsGroupToneMapping::toneMapName = nullptr;
}

bool
OptionsGroupToneMapping::parseColor3(int argc, char **argv, ColorRgb &value) {
    if ( argc < 3 || argv == nullptr || argv[0] == nullptr || argv[1] == nullptr || argv[2] == nullptr ) {
        return false;
    }
    char *endPointer = nullptr;
    const double r = strtod(argv[0], &endPointer);
    if ( endPointer == argv[0] || *endPointer != '\0' ) {
        return false;
    }
    const double g = strtod(argv[1], &endPointer);
    if ( endPointer == argv[1] || *endPointer != '\0' ) {
        return false;
    }
    const double b = strtod(argv[2], &endPointer);
    if ( endPointer == argv[2] || *endPointer != '\0' ) {
        return false;
    }
    value = ColorRgb(r, g, b);
    return true;
}

bool
OptionsGroupToneMapping::parseCieXy(int argc, char **argv, Vector3D &value) {
    if ( argc < 2 || argv == nullptr || argv[0] == nullptr || argv[1] == nullptr ) {
        return false;
    }
    char *endPointer = nullptr;
    value.x = strtof(argv[0], &endPointer);
    if ( endPointer == argv[0] || *endPointer != '\0' ) {
        return false;
    }
    value.y = strtof(argv[1], &endPointer);
    if ( endPointer == argv[1] || *endPointer != '\0' ) {
        return false;
    }
    value.z = 0.0;
    return true;
}
