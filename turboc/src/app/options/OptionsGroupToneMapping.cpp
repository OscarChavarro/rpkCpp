#include <stdlib.h>
#include <cstrings>

#include "java/lang/Math.h"
#include "common/Error.h"
#include "common/Cie.h"
#include "common/commandLineOptions/OptionParser.h"
#include "common/commandLineOptions/TypedOption.h"
#include "tonemap/ToneMap.h"
#include "app/options/OptionsGroupToneMapping.h"

char OptionsGroupToneMapping::toneMappingMethodsString[TONE_MAP_MTHS_STR_LEN];
char *OptionsGroupToneMapping::toneMapName = NULL;
ToneMappingContext *OptionsGroupToneMapping::toneMapOptions = NULL;

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
    if ( toneMapOptions == NULL ) {
        Error::fatal(-1, "CmdLineToneMppngOptsGrp::brightnessAdjustOption", "ToneMappingContext not set");
    }
    (*toneMapOptions).pow_bright_adjust = Math::pow(2.0f, (*toneMapOptions).brightness_adjust);
}

void
OptionsGroupToneMapping::redChromaOption(Vector3D &value) {
    if ( toneMapOptions == NULL ) {
        Error::fatal(-1, "CmdLineToneMppngOptsGrp::redChromaOption", "ToneMappingContext not set");
    }
    (*toneMapOptions).xr = value.x;
    (*toneMapOptions).yr = value.y;
    Cie::cmptClrConvXforms(
        (*toneMapOptions).xr, (*toneMapOptions).yr,
        (*toneMapOptions).xg, (*toneMapOptions).yg,
        (*toneMapOptions).xb, (*toneMapOptions).yb,
        (*toneMapOptions).xw, (*toneMapOptions).yw);
}

void
OptionsGroupToneMapping::greenChromaOption(Vector3D &value) {
    if ( toneMapOptions == NULL ) {
        Error::fatal(-1, "CmdLineToneMppngOptsGrp::greenChromaOption", "ToneMappingContext not set");
    }
    (*toneMapOptions).xg = value.x;
    (*toneMapOptions).yg = value.y;
    Cie::cmptClrConvXforms(
        (*toneMapOptions).xr, (*toneMapOptions).yr,
        (*toneMapOptions).xg, (*toneMapOptions).yg,
        (*toneMapOptions).xb, (*toneMapOptions).yb,
        (*toneMapOptions).xw, (*toneMapOptions).yw);
}

void
OptionsGroupToneMapping::blueChromaOption(Vector3D &value) {
    if ( toneMapOptions == NULL ) {
        Error::fatal(-1, "CmdLineToneMppngOptsGrp::blueChromaOption", "ToneMappingContext not set");
    }
    (*toneMapOptions).xb = value.x;
    (*toneMapOptions).yb = value.y;
    Cie::cmptClrConvXforms(
        (*toneMapOptions).xr, (*toneMapOptions).yr,
        (*toneMapOptions).xg, (*toneMapOptions).yg,
        (*toneMapOptions).xb, (*toneMapOptions).yb,
        (*toneMapOptions).xw, (*toneMapOptions).yw);
}

void
OptionsGroupToneMapping::whiteChromaOption(Vector3D &value) {
    if ( toneMapOptions == NULL ) {
        Error::fatal(-1, "CmdLineToneMppngOptsGrp::whiteChromaOption", "ToneMappingContext not set");
    }
    (*toneMapOptions).xw = value.x;
    (*toneMapOptions).yw = value.y;
    Cie::cmptClrConvXforms(
        (*toneMapOptions).xr, (*toneMapOptions).yr,
        (*toneMapOptions).xg, (*toneMapOptions).yg,
        (*toneMapOptions).xb, (*toneMapOptions).yb,
        (*toneMapOptions).xw, (*toneMapOptions).yw);
}

void
OptionsGroupToneMapping::tonMppCmdLinOptDesAdaMetOpt(char *&name) {
    if ( toneMapOptions == NULL ) {
        Error::fatal(-1, "CmdLineToneMppngOptsGrp::tonMppCmdLinOptDesAdaMetOpt", "ToneMappingContext not set");
    }
    if ( strncasecmp(name, "average", 2) == 0 ) {
        (*toneMapOptions).staticAdaptationMethod = TMA_AVERAGE;
    } else if ( strncasecmp(name, "median", 2) == 0 ) {
        (*toneMapOptions).staticAdaptationMethod = TMA_MEDIAN;
    } else {
        Error::error(NULL, "Invalid adaptation estimate method '%s'", name);
    }
}

void
OptionsGroupToneMapping::gammaOption(float &gam) {
    if ( toneMapOptions == NULL ) {
        Error::fatal(-1, "CmdLineToneMppngOptsGrp::gammaOption", "ToneMappingContext not set");
    }
    (*toneMapOptions).gamma.set(gam, gam, gam);
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
    Vector3D redChromaticityValue(0.0, 0.0, 0.0);
    Vector3D greenChromaticityValue(0.0, 0.0, 0.0);
    Vector3D blueChromaticityValue(0.0, 0.0, 0.0);
    Vector3D whiteChromaticityValue(0.0, 0.0, 0.0);
    TypedOption<char *> toneMappingOpt("-tonemapping", &toneMapMethodName, 1, OptionsGroupToneMapping::toneMappingMethodOption, NULL);
    TypedOption<float> brightnessAdjustOpt("-brightness-adjust", &toneMapOptionsContext.brightness_adjust, 1, OptionsGroupToneMapping::brightnessAdjustOption, NULL);
    TypedOption<char *> adaptOpt("-adapt", &adaptMethodName, 1, OptionsGroupToneMapping::tonMppCmdLinOptDesAdaMetOpt, NULL);
    TypedOption<float> lwaOpt("-lwa", &toneMapOptionsContext.realWorldAdaptionLuminance, 1, NULL, NULL);
    TypedOption<float> ldmaxOpt("-ldmax", &toneMapOptionsContext.maximumDisplayLuminance, 1, NULL, NULL);
    TypedOption<float> cmaxOpt("-cmax", &toneMapOptionsContext.maximumDisplayContrast, 1, NULL, NULL);
    TypedOption<float> gammaOpt("-gamma", &toneMapOptionsContext.gamma.r, 1, OptionsGroupToneMapping::gammaOption, NULL);
    TypedOption<ColorRgb> rgbGammaOpt("-rgbgamma", &toneMapOptionsContext.gamma, 3, NULL, OptionsGroupToneMapping::parseColor3);
    TypedOption<Vector3D> redOpt("-red", &redChromaticityValue, 2, OptionsGroupToneMapping::redChromaOption, OptionsGroupToneMapping::parseCieXy);
    TypedOption<Vector3D> greenOpt("-green", &greenChromaticityValue, 2, OptionsGroupToneMapping::greenChromaOption, OptionsGroupToneMapping::parseCieXy);
    TypedOption<Vector3D> blueOpt("-blue", &blueChromaticityValue, 2, OptionsGroupToneMapping::blueChromaOption, OptionsGroupToneMapping::parseCieXy);
    TypedOption<Vector3D> whiteOpt("-white", &whiteChromaticityValue, 2, OptionsGroupToneMapping::whiteChromaOption, OptionsGroupToneMapping::parseCieXy);
    OptionBase toneMappingOptions[] = {
        REGISTER_OPTION(char *, toneMappingOpt, 4),
        REGISTER_OPTION(float, brightnessAdjustOpt, 4),
        REGISTER_OPTION(char *, adaptOpt, 5),
        REGISTER_OPTION(float, lwaOpt, 3),
        REGISTER_OPTION(float, ldmaxOpt, 5),
        REGISTER_OPTION(float, cmaxOpt, 4),
        REGISTER_OPTION(float, gammaOpt, 4),
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
    OptionsGroupToneMapping::toneMapOptions = NULL;
    OptionsGroupToneMapping::toneMapName = NULL;
}

bool
OptionsGroupToneMapping::parseColor3(int argc, char **argv, ColorRgb &value) {
    if ( argc < 3 || argv == NULL || argv[0] == NULL || argv[1] == NULL || argv[2] == NULL ) {
        return false;
    }
    char *endPointer = NULL;
    value.r = strtof(argv[0], &endPointer);
    if ( endPointer == argv[0] || *endPointer != '\0' ) {
        return false;
    }
    value.g = strtof(argv[1], &endPointer);
    if ( endPointer == argv[1] || *endPointer != '\0' ) {
        return false;
    }
    value.b = strtof(argv[2], &endPointer);
    if ( endPointer == argv[2] || *endPointer != '\0' ) {
        return false;
    }
    return true;
}

bool
OptionsGroupToneMapping::parseCieXy(int argc, char **argv, Vector3D &value) {
    if ( argc < 2 || argv == NULL || argv[0] == NULL || argv[1] == NULL ) {
        return false;
    }
    char *endPointer = NULL;
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
