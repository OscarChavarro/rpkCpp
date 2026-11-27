#include <stdlib.h>
#include <strings.h>

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/logging/LoggerFormat.h"
#include "vsdk/toolkit/common/color/Cie.h"
#include "vsdk/toolkit/common/commandLineOptions/OptionParser.h"
#include "vsdk/toolkit/common/commandLineOptions/TypedOption.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"
#include "app/options/OptionsGroupToneMapping.h"

char OptionsGroupToneMapping::toneMappingMethodsString[TONE_MAP_MTHS_STR_LEN];
char *OptionsGroupToneMapping::toneMapName = NULL;
ToneMappingContext *OptionsGroupToneMapping::toneMapOptions = NULL;
double OptionsGroupToneMapping::gammaValue = 0.0;

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
    if ( toneMapOptions == NULL ) {
        Logger::reportMessage("OptionsGroupToneMapping", Logger::FATAL_ERROR, "CmdLineToneMppngOptsGrp::brightnessAdjustOption", "ToneMappingContext not set");
    }
    (*toneMapOptions).pow_bright_adjust = Math::pow(2.0, (*toneMapOptions).brightness_adjust);
}

void
OptionsGroupToneMapping::redChromaOption(Vector3Dd &value) {
    if ( toneMapOptions == NULL ) {
        Logger::reportMessage("OptionsGroupToneMapping", Logger::FATAL_ERROR, "CmdLineToneMppngOptsGrp::redChromaOption", "ToneMappingContext not set");
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
    if ( toneMapOptions == NULL ) {
        Logger::reportMessage("OptionsGroupToneMapping", Logger::FATAL_ERROR, "CmdLineToneMppngOptsGrp::greenChromaOption", "ToneMappingContext not set");
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
    if ( toneMapOptions == NULL ) {
        Logger::reportMessage("OptionsGroupToneMapping", Logger::FATAL_ERROR, "CmdLineToneMppngOptsGrp::blueChromaOption", "ToneMappingContext not set");
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
    if ( toneMapOptions == NULL ) {
        Logger::reportMessage("OptionsGroupToneMapping", Logger::FATAL_ERROR, "CmdLineToneMppngOptsGrp::whiteChromaOption", "ToneMappingContext not set");
    }
    const Cie &c = (*toneMapOptions).getColorSpace();
    (*toneMapOptions).setColorSpace(Cie(
        c.getRedX(), c.getRedY(),
        c.getGreenX(), c.getGreenY(),
        c.getBlueX(), c.getBlueY(),
        value.x(), value.y()));
}

void
OptionsGroupToneMapping::tonMppCmdLinOptDesAdaMetOpt(char *&name) {
    if ( toneMapOptions == NULL ) {
        Logger::reportMessage("OptionsGroupToneMapping", Logger::FATAL_ERROR, "CmdLineToneMppngOptsGrp::tonMppCmdLinOptDesAdaMetOpt", "ToneMappingContext not set");
    }
    if ( strncasecmp(name, "average", 2) == 0 ) {
        (*toneMapOptions).staticAdaptationMethod = TMA_AVERAGE;
    } else if ( strncasecmp(name, "median", 2) == 0 ) {
        (*toneMapOptions).staticAdaptationMethod = TMA_MEDIAN;
    } else {
        Logger::reportMessage("OptionsGroupToneMapping", Logger::ERROR, "", LoggerFormat::format("Invalid adaptation estimate method '%s'", name));
    }
}

void
OptionsGroupToneMapping::gammaOption(double &gam) {
    if ( toneMapOptions == NULL ) {
        Logger::reportMessage("OptionsGroupToneMapping", Logger::FATAL_ERROR, "CmdLineToneMppngOptsGrp::gammaOption", "ToneMappingContext not set");
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
    char *toneMapMethodName = NULL;
    char *adaptMethodName = NULL;
    Vector3Dd redChromaticityValue(0.0, 0.0, 0.0);
    Vector3Dd greenChromaticityValue(0.0, 0.0, 0.0);
    Vector3Dd blueChromaticityValue(0.0, 0.0, 0.0);
    Vector3Dd whiteChromaticityValue(0.0, 0.0, 0.0);
    TypedOption<char *> toneMappingOpt("-tonemapping", &toneMapMethodName, 1, OptionsGroupToneMapping::toneMappingMethodOption, NULL);
    TypedOption<double> brightnessAdjustOpt("-brightness-adjust", &toneMapOptionsContext.brightness_adjust, 1, OptionsGroupToneMapping::brightnessAdjustOption, NULL);
    TypedOption<char *> adaptOpt("-adapt", &adaptMethodName, 1, OptionsGroupToneMapping::tonMppCmdLinOptDesAdaMetOpt, NULL);
    TypedOption<double> lwaOpt("-lwa", &toneMapOptionsContext.realWorldAdaptionLuminance, 1, NULL, NULL);
    TypedOption<double> ldmaxOpt("-ldmax", &toneMapOptionsContext.maximumDisplayLuminance, 1, NULL, NULL);
    TypedOption<double> cmaxOpt("-cmax", &toneMapOptionsContext.maximumDisplayContrast, 1, NULL, NULL);
    gammaValue = toneMapOptionsContext.gamma.getR();
    TypedOption<double> gammaOpt("-gamma", &gammaValue, 1, OptionsGroupToneMapping::gammaOption, NULL);
    TypedOption<ColorRgb> rgbGammaOpt("-rgbgamma", &toneMapOptionsContext.gamma, 3, NULL, OptionsGroupToneMapping::parseColor3);
    TypedOption<Vector3Dd> redOpt("-red", &redChromaticityValue, 2, OptionsGroupToneMapping::redChromaOption, OptionsGroupToneMapping::parseCieXy);
    TypedOption<Vector3Dd> greenOpt("-green", &greenChromaticityValue, 2, OptionsGroupToneMapping::greenChromaOption, OptionsGroupToneMapping::parseCieXy);
    TypedOption<Vector3Dd> blueOpt("-blue", &blueChromaticityValue, 2, OptionsGroupToneMapping::blueChromaOption, OptionsGroupToneMapping::parseCieXy);
    TypedOption<Vector3Dd> whiteOpt("-white", &whiteChromaticityValue, 2, OptionsGroupToneMapping::whiteChromaOption, OptionsGroupToneMapping::parseCieXy);
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
    OptionsGroupToneMapping::toneMapOptions = NULL;
    OptionsGroupToneMapping::toneMapName = NULL;
}

bool
OptionsGroupToneMapping::parseColor3(int argc, char **argv, ColorRgb &value) {
    if ( argc < 3 || argv == NULL || argv[0] == NULL || argv[1] == NULL || argv[2] == NULL ) {
        return false;
    }
    char *endPointer = NULL;
    double r = strtod(argv[0], &endPointer);
    if ( endPointer == argv[0] || *endPointer != '\0' ) {
        return false;
    }
    double g = strtod(argv[1], &endPointer);
    if ( endPointer == argv[1] || *endPointer != '\0' ) {
        return false;
    }
    double b = strtod(argv[2], &endPointer);
    if ( endPointer == argv[2] || *endPointer != '\0' ) {
        return false;
    }
    value = ColorRgb(r, g, b);
    return true;
}

bool
OptionsGroupToneMapping::parseCieXy(int argc, char **argv, Vector3Dd &value) {
    if ( argc < 2 || argv == NULL || argv[0] == NULL || argv[1] == NULL ) {
        return false;
    }
    char *endPointer = NULL;
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
