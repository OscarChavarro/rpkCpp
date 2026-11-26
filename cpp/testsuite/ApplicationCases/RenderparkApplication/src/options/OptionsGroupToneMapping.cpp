#include <cstdlib>
#include <cstring>

#include "java/lang/Math.h"
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
OptionsGroupToneMapping::brightnessAdjustOption(double & /*value*/) {
    if ( toneMapOptions == nullptr ) {
        Logger::reportMessage("OptionsGroupToneMapping", Logger::FATAL_ERROR, "CommandLineToneMappingOptionsGroup::brightnessAdjustOption", "ToneMappingContext not set");
    }
    (*toneMapOptions).pow_bright_adjust = static_cast<double>(java::Math::pow(2.0, static_cast<double>((*toneMapOptions).brightness_adjust)));
}

void
OptionsGroupToneMapping::redChromaOption(Vector3Dd &value) {
    if ( toneMapOptions == nullptr ) {
        Logger::reportMessage("OptionsGroupToneMapping", Logger::FATAL_ERROR, "CommandLineToneMappingOptionsGroup::redChromaOption", "ToneMappingContext not set");
    }
    const Cie &c = (*toneMapOptions).getColorSpace();
    (*toneMapOptions).setColorSpace(Cie(
        value.x(), value.y(),
        c.getGreenX(), c.getGreenY(),
        c.getBlueX(), c.getBlueY(),
        c.getWhiteX(), c.getWhiteY()));
}

void
OptionsGroupToneMapping::greenChromaOption(Vector3Dd &value) {
    if ( toneMapOptions == nullptr ) {
        Logger::reportMessage("OptionsGroupToneMapping", Logger::FATAL_ERROR, "CommandLineToneMappingOptionsGroup::greenChromaOption", "ToneMappingContext not set");
    }
    const Cie &c = (*toneMapOptions).getColorSpace();
    (*toneMapOptions).setColorSpace(Cie(
        c.getRedX(), c.getRedY(),
        value.x(), value.y(),
        c.getBlueX(), c.getBlueY(),
        c.getWhiteX(), c.getWhiteY()));
}

void
OptionsGroupToneMapping::blueChromaOption(Vector3Dd &value) {
    if ( toneMapOptions == nullptr ) {
        Logger::reportMessage("OptionsGroupToneMapping", Logger::FATAL_ERROR, "CommandLineToneMappingOptionsGroup::blueChromaOption", "ToneMappingContext not set");
    }
    const Cie &c = (*toneMapOptions).getColorSpace();
    (*toneMapOptions).setColorSpace(Cie(
        c.getRedX(), c.getRedY(),
        c.getGreenX(), c.getGreenY(),
        value.x(), value.y(),
        c.getWhiteX(), c.getWhiteY()));
}

void
OptionsGroupToneMapping::whiteChromaOption(Vector3Dd &value) {
    if ( toneMapOptions == nullptr ) {
        Logger::reportMessage("OptionsGroupToneMapping", Logger::FATAL_ERROR, "CommandLineToneMappingOptionsGroup::whiteChromaOption", "ToneMappingContext not set");
    }
    const Cie &c = (*toneMapOptions).getColorSpace();
    (*toneMapOptions).setColorSpace(Cie(
        c.getRedX(), c.getRedY(),
        c.getGreenX(), c.getGreenY(),
        c.getBlueX(), c.getBlueY(),
        value.x(), value.y()));
}

void
OptionsGroupToneMapping::toneMappingCommandLineOptionDescAdaptMethodOption(char *&name) {
    if ( toneMapOptions == nullptr ) {
        Logger::reportMessage("OptionsGroupToneMapping", Logger::FATAL_ERROR, "CommandLineToneMappingOptionsGroup::toneMappingCommandLineOptionDescAdaptMethodOption", "ToneMappingContext not set");
    }
    if ( strncasecmp(name, "average", 2) == 0 ) {
        (*toneMapOptions).staticAdaptationMethod = ToneMapAdaptationMethod::TMA_AVERAGE;
    } else if ( strncasecmp(name, "median", 2) == 0 ) {
        (*toneMapOptions).staticAdaptationMethod = ToneMapAdaptationMethod::TMA_MEDIAN;
    } else {
        Logger::reportMessage("OptionsGroupToneMapping", Logger::ERROR, "", java::String::format("Invalid adaptation estimate method '%s'", name));
    }
}

void
OptionsGroupToneMapping::gammaOption(double &gam) {
    if ( toneMapOptions == nullptr ) {
        Logger::reportMessage("OptionsGroupToneMapping", Logger::FATAL_ERROR, "CommandLineToneMappingOptionsGroup::gammaOption", "ToneMappingContext not set");
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
    Vector3Dd redChromaticityValue(0.0, 0.0, 0.0);
    Vector3Dd greenChromaticityValue(0.0, 0.0, 0.0);
    Vector3Dd blueChromaticityValue(0.0, 0.0, 0.0);
    Vector3Dd whiteChromaticityValue(0.0, 0.0, 0.0);
    double gammaScalar = toneMapOptionsContext.gamma.getR();
    TypedOption<char *> toneMappingOpt = {"-tonemapping", &toneMapMethodName, 1, OptionsGroupToneMapping::toneMappingMethodOption, nullptr};
    TypedOption<double> brightnessAdjustOpt = {"-brightness-adjust", &toneMapOptionsContext.brightness_adjust, 1, OptionsGroupToneMapping::brightnessAdjustOption, nullptr};
    TypedOption<char *> adaptOpt = {"-adapt", &adaptMethodName, 1, OptionsGroupToneMapping::toneMappingCommandLineOptionDescAdaptMethodOption, nullptr};
    TypedOption<double> lwaOpt = {"-lwa", &toneMapOptionsContext.realWorldAdaptionLuminance, 1, nullptr, nullptr};
    TypedOption<double> ldmaxOpt = {"-ldmax", &toneMapOptionsContext.maximumDisplayLuminance, 1, nullptr, nullptr};
    TypedOption<double> cmaxOpt = {"-cmax", &toneMapOptionsContext.maximumDisplayContrast, 1, nullptr, nullptr};
    TypedOption<double> gammaOpt = {"-gamma", &gammaScalar, 1, OptionsGroupToneMapping::gammaOption, nullptr};
    TypedOption<ColorRgb> rgbGammaOpt = {"-rgbgamma", &toneMapOptionsContext.gamma, 3, nullptr, OptionsGroupToneMapping::parseColor3};
    TypedOption<Vector3Dd> redOpt = {"-red", &redChromaticityValue, 2, OptionsGroupToneMapping::redChromaOption, OptionsGroupToneMapping::parseCieXy};
    TypedOption<Vector3Dd> greenOpt = {"-green", &greenChromaticityValue, 2, OptionsGroupToneMapping::greenChromaOption, OptionsGroupToneMapping::parseCieXy};
    TypedOption<Vector3Dd> blueOpt = {"-blue", &blueChromaticityValue, 2, OptionsGroupToneMapping::blueChromaOption, OptionsGroupToneMapping::parseCieXy};
    TypedOption<Vector3Dd> whiteOpt = {"-white", &whiteChromaticityValue, 2, OptionsGroupToneMapping::whiteChromaOption, OptionsGroupToneMapping::parseCieXy};
    OptionBase toneMappingOptions[] = {
        REGISTER_OPTION(char *, toneMappingOpt, 4),
        REGISTER_OPTION(double, brightnessAdjustOpt, 4),
        REGISTER_OPTION(char *, adaptOpt, 5),
        REGISTER_OPTION(double, lwaOpt, 3),
        REGISTER_OPTION(double, ldmaxOpt, 5),
        REGISTER_OPTION(double, cmaxOpt, 4),
        REGISTER_OPTION(double, gammaOpt, 4),
        REGISTER_OPTION(ColorRgb, rgbGammaOpt, 4),
        REGISTER_OPTION(Vector3Dd, redOpt, 4),
        REGISTER_OPTION(Vector3Dd, greenOpt, 4),
        REGISTER_OPTION(Vector3Dd, blueOpt, 4),
        REGISTER_OPTION(Vector3Dd, whiteOpt, 4)
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
OptionsGroupToneMapping::parseCieXy(int argc, char **argv, Vector3Dd &value) {
    if ( argc < 2 || argv == nullptr || argv[0] == nullptr || argv[1] == nullptr ) {
        return false;
    }
    char *endPointer = nullptr;
    value = value.withX(strtod(argv[0], &endPointer));
    if ( endPointer == argv[0] || *endPointer != '\0' ) {
        return false;
    }
    value = value.withY(strtod(argv[1], &endPointer));
    if ( endPointer == argv[1] || *endPointer != '\0' ) {
        return false;
    }
    value = value.withZ(0.0);
    return true;
}
