#include <cstdlib>
#include <cstring>
#include <strings.h>

#include "java/lang/System.h"
#include "common/Error.h"
#include "common/RenderOptions.h"
#include "scene/ConstantColorBackground.h"
#include "tonemap/ToneMap.h"
#include "galerkin/GalerkinRadianceMethod.h"

#ifdef RAYTRACING_ENABLED
    #include "raycasting/simple/RayMatterState.h"
    #include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
    #include "raycasting/stochasticRaytracing/Hierarchy.h"
    #include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
    #include "raycasting/stochasticRaytracing/StochasticRelaxation.h"
    #include "raycasting/photonMap/PhotonMapState.h"
#endif

#include "app/options/BackgroundMode.h"
#include "common/commandLineOptions/OptionParser.h"
#include "common/commandLineOptions/TypedOption.h"
#include "app/options/BatchOptions.h"
#include "app/options/CommandLine.h"
#include "app/options/GeneralProgramOptions.h"

namespace {

template<typename T>
class EnumBinding {
  public:
    T *target;
    const EnumDesc *values;
};

class FixedStringBinding {
  public:
    char *target;
    int maxLength;
};

template<typename T>
bool parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding) {
    if ( argc < 1 || argv == nullptr || argv[0] == nullptr || binding.target == nullptr || binding.values == nullptr ) {
        return false;
    }
    for ( int i = 0; binding.values[i].name != nullptr; i++ ) {
        if ( strncasecmp(argv[0], binding.values[i].name, binding.values[i].abbrev) == 0 ) {
            *binding.target = static_cast<T>(binding.values[i].value);
            return true;
        }
    }
    return false;
}

bool parseFixedStringBinding(int argc, char **argv, FixedStringBinding &binding) {
    if ( argc < 1 || argv == nullptr || argv[0] == nullptr || binding.target == nullptr || binding.maxLength <= 0 ) {
        return false;
    }
    strncpy(binding.target, argv[0], binding.maxLength);
    binding.target[binding.maxLength - 1] = '\0';
    return true;
}

bool parseVector3(int argc, char **argv, Vector3D &value) {
    if ( argc < 3 || argv == nullptr || argv[0] == nullptr || argv[1] == nullptr || argv[2] == nullptr ) {
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
    value.z = strtof(argv[2], &endPointer);
    if ( endPointer == argv[2] || *endPointer != '\0' ) {
        return false;
    }
    return true;
}

bool parseColor3(int argc, char **argv, ColorRgb &value) {
    if ( argc < 3 || argv == nullptr || argv[0] == nullptr || argv[1] == nullptr || argv[2] == nullptr ) {
        return false;
    }
    char *endPointer = nullptr;
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

bool parseCieXy(int argc, char **argv, Vector3D &value) {
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

bool parseBoolInt(int argc, char **argv, int &value) {
    if ( argc < 1 || argv == nullptr || argv[0] == nullptr ) {
        return false;
    }
    if ( strcasecmp(argv[0], "true") == 0 || strcasecmp(argv[0], "yes") == 0 || strcmp(argv[0], "1") == 0 ) {
        value = 1;
        return true;
    }
    if ( strcasecmp(argv[0], "false") == 0 || strcasecmp(argv[0], "no") == 0 || strcmp(argv[0], "0") == 0 ) {
        value = 0;
        return true;
    }
    return false;
}

void setIntTrue(int &value) {
    value = 1;
}

void setIntFalse(int &value) {
    value = 0;
}

}

const Vector3D CommandLine::DEFAULT_CAMERA_EYE_POSITION(10.0, 0.0, 0.0);
const Vector3D CommandLine::DEFAULT_CAMERA_LOOK_POSITION(0.0, 0.0, 0.0);
const Vector3D CommandLine::DEFAULT_CAMERA_UP_DIRECTION(0.0, 0.0, 1.0);
const ColorRgb CommandLine::DEFAULT_BACKGROUND_COLOR(0.0, 0.0, 0.0);

int CommandLine::numberOfQuarterCircleDivisions = CommandLine::DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
int CommandLine::fileOptionsForceOneSidedSurfaces = 0;
int CommandLine::yesValue = 1;
int CommandLine::noValue = 0;
int CommandLine::outputImageWidth = 1920;
int CommandLine::outputImageHeight = 1080;
int CommandLine::glutDebugEnabled = false;
Camera CommandLine::cameraState;
BackgroundMode CommandLine::backgroundMode = BackgroundMode::NONE;
ColorRgb CommandLine::backgroundColor = CommandLine::DEFAULT_BACKGROUND_COLOR;
int CommandLine::trueValue = true;
int CommandLine::falseValue = false;

char CommandLine::toneMappingMethodsString[CommandLine::TONE_MAPPING_METHODS_STRING_LENGTH];
float CommandLine::redChromaticity[2];
float CommandLine::greenChromaticity[2];
float CommandLine::blueChromaticity[2];
float CommandLine::whiteChromaticity[2];
char *CommandLine::toneMapName = nullptr;
ToneMappingContext *CommandLine::toneMapOptions = nullptr;
char *CommandLine::radianceMethodsString = nullptr;
RenderOptions CommandLine::renderOptionsState;
ColorRgb CommandLine::outlineColor;
BatchOptions CommandLine::batchOptionsState;

ColorRgb
CommandLine::commandLineDefaultBackgroundColor() {
    return DEFAULT_BACKGROUND_COLOR;
}

Background *
CommandLine::commandLineCreateBackground() {
    if ( backgroundMode == BackgroundMode::SOLID ) {
        return new ConstantColorBackground(backgroundColor);
    }
    return nullptr;
}

bool
CommandLine::commandLineParseFloat(const char *text, float *value) {
    if ( text == nullptr || value == nullptr ) {
        return false;
    }

    char *endPointer = nullptr;
    const float parsedValue = strtof(text, &endPointer);
    if ( endPointer == text || *endPointer != '\0' ) {
        return false;
    }

    *value = parsedValue;
    return true;
}

bool
CommandLine::commandLineParseBackgroundColor(const char *rArg, const char *gArg, const char *bArg, ColorRgb *color) {
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    if ( !CommandLine::commandLineParseFloat(rArg, &red)
         || !CommandLine::commandLineParseFloat(gArg, &green)
         || !CommandLine::commandLineParseFloat(bArg, &blue) ) {
        return false;
    }

    if ( red < 0.0f || red > 1.0f || green < 0.0f || green > 1.0f || blue < 0.0f || blue > 1.0f ) {
        return false;
    }

    color->set(red, green, blue);
    return true;
}

void
CommandLine::commandLineParseBackgroundOption(int *argc, char **argv) {
    int writeIndex = 0;
    int readIndex = 0;
    while ( readIndex < *argc ) {
        const char *argument = argv[readIndex];
        if ( argument == nullptr || strcmp(argument, "-background") != 0 ) {
            argv[writeIndex++] = argv[readIndex++];
            continue;
        }

        if ( readIndex + 1 >= *argc ) {
            java::System::err.printf("Option '-background' requires a mode. Supported mode: solid.\n");
            readIndex += 1;
            continue;
        }

        const char *mode = argv[readIndex + 1];
        if ( strcasecmp(mode, "solid") != 0 ) {
            java::System::err.printf(
                "Invalid background mode '%s'. Expected '-background solid <r> <g> <b>'.\n",
                mode);
            readIndex += 2;
            continue;
        }

        if ( readIndex + 4 >= *argc ) {
            java::System::err.printf(
                "Option '-background solid' requires three values in range [0.0, 1.0].\n");
            readIndex += 2;
            continue;
        }

        ColorRgb parsedColor;
        if ( !CommandLine::commandLineParseBackgroundColor(
                 argv[readIndex + 2],
                 argv[readIndex + 3],
                 argv[readIndex + 4],
                 &parsedColor) ) {
            java::System::err.printf(
                "Invalid '-background solid' color. Use '-background solid <r> <g> <b>' with values in [0.0, 1.0].\n");
        } else {
            backgroundMode = BackgroundMode::SOLID;
            backgroundColor = parsedColor;
        }
        readIndex += 5;
    }

    while ( writeIndex < *argc ) {
        argv[writeIndex++] = nullptr;
    }
    *argc = writeIndex;
}

void
CommandLine::mainForceOneSidedOption(int &value) {
    fileOptionsForceOneSidedSurfaces = value;
}

void
CommandLine::mainMonochromeOption(int &value) {
    numberOfQuarterCircleDivisions = value;
}

void
CommandLine::commandLineImageWidthOption(int &value) {
    outputImageWidth = value;
}

void
CommandLine::commandLineImageHeightOption(int &value) {
    outputImageHeight = value;
}

void
CommandLine::commandLineGeneralProgramParseOptions(
        int *argc,
        char **argv,
        bool *oneSidedSurfaces,
        int *conicSubDivisions,
        int *imageOutputWidth,
        int *imageOutputHeight,
        bool *glutDebugEnabledOut)
{
    GeneralProgramOptionsRegistry optionsRegistry(
        CommandLine::mainForceOneSidedOption,
        CommandLine::mainMonochromeOption,
        setIntTrue);

    AppOptions appOptions;
    appOptions.width = outputImageWidth;
    appOptions.height = outputImageHeight;
    appOptions.nqcdivs = numberOfQuarterCircleDivisions;
    appOptions.yesValue = 1;
    appOptions.noValue = 0;
    appOptions.debug = 0;

    fileOptionsForceOneSidedSurfaces = DEFAULT_FORCE_ONE_SIDED;
    numberOfQuarterCircleDivisions = DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
    backgroundMode = BackgroundMode::NONE;
    backgroundColor = DEFAULT_BACKGROUND_COLOR;
    CommandLine::glutDebugEnabled = appOptions.debug;
    CommandLine::commandLineParseBackgroundOption(argc, argv);
    OptionGroup generalGroups[] = {
        OptionGroup("global", optionsRegistry.entries(), optionsRegistry.count())
    };
    OptionParser<OptionBase>::parse(argc, argv, generalGroups, 1, &appOptions);

    outputImageWidth = appOptions.width;
    outputImageHeight = appOptions.height;
    numberOfQuarterCircleDivisions = appOptions.nqcdivs;
    CommandLine::glutDebugEnabled = appOptions.debug;

    if ( fileOptionsForceOneSidedSurfaces != 0 ) {
        *oneSidedSurfaces = true;
    } else {
        *oneSidedSurfaces = false;
    }
    *conicSubDivisions = numberOfQuarterCircleDivisions;
    *imageOutputWidth = outputImageWidth;
    *imageOutputHeight = outputImageHeight;
    *glutDebugEnabledOut = CommandLine::glutDebugEnabled;

#ifndef OPEN_GL_ENABLED
    if ( CommandLine::glutDebugEnabled ) {
        java::System::err.printf(
            "ERROR: Option '-glutDebug' requires OpenGL support. Recompile with -DOPEN_GL_ENABLED=ON.\n");
        java::System::err.flush();
        java::System::exit(1);
    }
#endif
}

void
CommandLine::cameraSetEyePositionOption(Vector3D &val) {
    cameraState.setEyePosition(val.x, val.y, val.z);
}

void
CommandLine::cameraSetLookPositionOption(Vector3D &val) {
    cameraState.setLookPosition(val.x, val.y, val.z);
}

void
CommandLine::cameraSetUpDirectionOption(Vector3D &val) {
    cameraState.setUpDirection(val.x, val.y, val.z);
}

void
CommandLine::cameraSetFieldOfViewOption(float &val) {
    cameraState.setFieldOfView(val);
}

void
CommandLine::cameraDefaults(Camera *camera, int imageWidth, int imageHeight) {
    Vector3D eyePosition = DEFAULT_CAMERA_EYE_POSITION;
    Vector3D lookPosition = DEFAULT_CAMERA_LOOK_POSITION;
    Vector3D upDirection = DEFAULT_CAMERA_UP_DIRECTION;
    ColorRgb backgroundColorSelected = DEFAULT_BACKGROUND_COLOR;

    camera->set(
        &eyePosition,
        &lookPosition,
        &upDirection,
        DEFAULT_CAMERA_FIELD_OF_VIEW,
        imageWidth,
        imageHeight,
        &backgroundColorSelected);
}

void
CommandLine::cameraParseOptions(
        int *argc,
        char **argv,
        Camera *camera,
        int imageWidth,
        int imageHeight)
{
    TypedOption<Vector3D> eyePointOpt = {"-eyepoint", &cameraState.eyePosition, 3, CommandLine::cameraSetEyePositionOption, parseVector3};
    TypedOption<Vector3D> centerOpt = {"-center", &cameraState.lookPosition, 3, CommandLine::cameraSetLookPositionOption, parseVector3};
    TypedOption<Vector3D> upDirOpt = {"-updir", &cameraState.upDirection, 3, CommandLine::cameraSetUpDirectionOption, parseVector3};
    TypedOption<float> fovOpt = {"-fov", &cameraState.fieldOfVision, 1, CommandLine::cameraSetFieldOfViewOption, nullptr};
    OptionBase cameraOptions[] = {
        REGISTER_OPTION(Vector3D, eyePointOpt, 4),
        REGISTER_OPTION(Vector3D, centerOpt, 4),
        REGISTER_OPTION(Vector3D, upDirOpt, 3),
        REGISTER_OPTION(float, fovOpt, 4)
    };

    CommandLine::cameraDefaults(&cameraState, imageWidth, imageHeight);
    OptionParser<OptionBase>::parse(argc, argv, cameraOptions, 4);
    *camera = cameraState;
}

void
CommandLine::iterationMethodOption(char *&name) {

    if ( strncasecmp(name, "jacobi", 2) == 0 ) {
        GalerkinRadianceMethod::galerkinState.galerkinIterationMethod = GalerkinIterationMethod::JACOBI;
    } else if ( strncasecmp(name, "gaussseidel", 2) == 0 ) {
        GalerkinRadianceMethod::galerkinState.galerkinIterationMethod = GalerkinIterationMethod::GAUSS_SEIDEL;
    } else if ( strncasecmp(name, "southwell", 2) == 0 ) {
        GalerkinRadianceMethod::galerkinState.galerkinIterationMethod = GalerkinIterationMethod::SOUTH_WELL;
    } else {
        Error::error(nullptr, "Invalid iteration method '%s'", name);
    }
}

void
CommandLine::hierarchicalOption(int &yesno) {

    if ( yesno != 0 ) {
        GalerkinRadianceMethod::galerkinState.hierarchical = true;
    } else {
        GalerkinRadianceMethod::galerkinState.hierarchical = false;
    }
}

void
CommandLine::lazyOption(int &yesno) {
    GalerkinRadianceMethod::galerkinState.lazyLinking = yesno;
}

void
CommandLine::clusteringOption(int &yesno) {
    GalerkinRadianceMethod::galerkinState.clustered = yesno;
}

void
CommandLine::importanceOption(int &yesno) {
    GalerkinRadianceMethod::galerkinState.importanceDriven = yesno;
}

void
CommandLine::ambientOption(int &yesno) {
    GalerkinRadianceMethod::galerkinState.useAmbientRadiance = yesno;
}

void
CommandLine::galerkinParseOptions(int *argc, char **argv) {
    char *iterationMethodName = nullptr;
    TypedOption<char *> iterationMethodOpt = {"-gr-iteration-method", &iterationMethodName, 1, CommandLine::iterationMethodOption, nullptr};
    TypedOption<int> grHierarchicalOpt = {"-gr-hierarchical", &trueValue, 0, CommandLine::hierarchicalOption, nullptr};
    TypedOption<int> grNotHierarchicalOpt = {"-gr-not-hierarchical", &falseValue, 0, CommandLine::hierarchicalOption, nullptr};
    TypedOption<int> grLazyOpt = {"-gr-lazy-linking", &trueValue, 0, CommandLine::lazyOption, nullptr};
    TypedOption<int> grNoLazyOpt = {"-gr-no-lazy-linking", &falseValue, 0, CommandLine::lazyOption, nullptr};
    TypedOption<int> grClusteringOpt = {"-gr-clustering", &trueValue, 0, CommandLine::clusteringOption, nullptr};
    TypedOption<int> grNoClusteringOpt = {"-gr-no-clustering", &falseValue, 0, CommandLine::clusteringOption, nullptr};
    TypedOption<int> grImportanceOpt = {"-gr-importance", &trueValue, 0, CommandLine::importanceOption, nullptr};
    TypedOption<int> grNoImportanceOpt = {"-gr-no-importance", &falseValue, 0, CommandLine::importanceOption, nullptr};
    TypedOption<int> grAmbientOpt = {"-gr-ambient", &trueValue, 0, CommandLine::ambientOption, nullptr};
    TypedOption<int> grNoAmbientOpt = {"-gr-no-ambient", &falseValue, 0, CommandLine::ambientOption, nullptr};
    TypedOption<float> grLinkErrorOpt = {"-gr-link-error-threshold", &GalerkinRadianceMethod::galerkinState.relLinkErrorThreshold, 1, nullptr, nullptr};
    TypedOption<float> grMinElemAreaOpt = {"-gr-min-elem-area", &GalerkinRadianceMethod::galerkinState.relMinElemArea, 1, nullptr, nullptr};
    OptionBase galerkinOptions[] = {
        REGISTER_OPTION(char *, iterationMethodOpt, 6),
        REGISTER_OPTION(int, grHierarchicalOpt, 6),
        REGISTER_OPTION(int, grNotHierarchicalOpt, 10),
        REGISTER_OPTION(int, grLazyOpt, 6),
        REGISTER_OPTION(int, grNoLazyOpt, 10),
        REGISTER_OPTION(int, grClusteringOpt, 6),
        REGISTER_OPTION(int, grNoClusteringOpt, 10),
        REGISTER_OPTION(int, grImportanceOpt, 6),
        REGISTER_OPTION(int, grNoImportanceOpt, 10),
        REGISTER_OPTION(int, grAmbientOpt, 6),
        REGISTER_OPTION(int, grNoAmbientOpt, 10),
        REGISTER_OPTION(float, grLinkErrorOpt, 6),
        REGISTER_OPTION(float, grMinElemAreaOpt, 6)
    };

    OptionParser<OptionBase>::parse(argc, argv, galerkinOptions, 13);
}

// Composes explanation for -tonemapping command line option
void
CommandLine::makeToneMappingMethodsString() {
    strcpy(toneMappingMethodsString,
       "-tonemapping <method>: Set tone mapping method\n"
       "\tmethods: Lightness            Lightness Mapping (default)\n"
       "\t         TumblinRushmeier     Tumblin/Rushmeier's Mapping\n"
       "\t         Ward                 Ward's Mapping\n"
       "\t         RevisedTR            Revised Tumblin/Rushmeier's Mapping\n"
       "\t         Ferwerda             Partial Ferwerda's Mapping");
}

void
CommandLine::toneMappingMethodOption(char *&name) {
    strcpy(toneMapName, name);
}

void
CommandLine::brightnessAdjustOption(float & /*value*/) {
    if ( toneMapOptions == nullptr ) {
        Error::fatal(-1, "CommandLine::brightnessAdjustOption", "ToneMappingContext not set");
    }
    (*toneMapOptions).pow_bright_adjust = java::Math::pow(2.0f, (*toneMapOptions).brightness_adjust);
}

void
CommandLine::redChromaOption(Vector3D &value) {
    if ( toneMapOptions == nullptr ) {
        Error::fatal(-1, "CommandLine::redChromaOption", "ToneMappingContext not set");
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
CommandLine::greenChromaOption(Vector3D &value) {
    if ( toneMapOptions == nullptr ) {
        Error::fatal(-1, "CommandLine::greenChromaOption", "ToneMappingContext not set");
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
CommandLine::blueChromaOption(Vector3D &value) {
    if ( toneMapOptions == nullptr ) {
        Error::fatal(-1, "CommandLine::blueChromaOption", "ToneMappingContext not set");
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
CommandLine::whiteChromaOption(Vector3D &value) {
    if ( toneMapOptions == nullptr ) {
        Error::fatal(-1, "CommandLine::whiteChromaOption", "ToneMappingContext not set");
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
CommandLine::toneMappingCommandLineOptionDescAdaptMethodOption(char *&name) {
    if ( toneMapOptions == nullptr ) {
        Error::fatal(-1, "CommandLine::toneMappingCommandLineOptionDescAdaptMethodOption", "ToneMappingContext not set");
    }

    if ( strncasecmp(name, "average", 2) == 0 ) {
        (*toneMapOptions).staticAdaptationMethod = ToneMapAdaptationMethod::TMA_AVERAGE;
    } else if ( strncasecmp(name, "median", 2) == 0 ) {
        (*toneMapOptions).staticAdaptationMethod = ToneMapAdaptationMethod::TMA_MEDIAN;
    } else {
        Error::error(nullptr, "Invalid adaptation estimate method '%s'", name);
    }
}

void
CommandLine::gammaOption(float &gam) {
    if ( toneMapOptions == nullptr ) {
        Error::fatal(-1, "CommandLine::gammaOption", "ToneMappingContext not set");
    }
    (*toneMapOptions).gamma.set(gam, gam, gam);
}

void
CommandLine::toneMapParseOptions(
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
    TypedOption<char *> toneMappingOpt = {"-tonemapping", &toneMapMethodName, 1, CommandLine::toneMappingMethodOption, nullptr};
    TypedOption<float> brightnessAdjustOpt = {"-brightness-adjust", &toneMapOptionsContext.brightness_adjust, 1, CommandLine::brightnessAdjustOption, nullptr};
    TypedOption<char *> adaptOpt = {"-adapt", &adaptMethodName, 1, CommandLine::toneMappingCommandLineOptionDescAdaptMethodOption, nullptr};
    TypedOption<float> lwaOpt = {"-lwa", &toneMapOptionsContext.realWorldAdaptionLuminance, 1, nullptr, nullptr};
    TypedOption<float> ldmaxOpt = {"-ldmax", &toneMapOptionsContext.maximumDisplayLuminance, 1, nullptr, nullptr};
    TypedOption<float> cmaxOpt = {"-cmax", &toneMapOptionsContext.maximumDisplayContrast, 1, nullptr, nullptr};
    TypedOption<float> gammaOpt = {"-gamma", &toneMapOptionsContext.gamma.r, 1, CommandLine::gammaOption, nullptr};
    TypedOption<ColorRgb> rgbGammaOpt = {"-rgbgamma", &toneMapOptionsContext.gamma, 3, nullptr, parseColor3};
    TypedOption<Vector3D> redOpt = {"-red", &redChromaticityValue, 2, CommandLine::redChromaOption, parseCieXy};
    TypedOption<Vector3D> greenOpt = {"-green", &greenChromaticityValue, 2, CommandLine::greenChromaOption, parseCieXy};
    TypedOption<Vector3D> blueOpt = {"-blue", &blueChromaticityValue, 2, CommandLine::blueChromaOption, parseCieXy};
    TypedOption<Vector3D> whiteOpt = {"-white", &whiteChromaticityValue, 2, CommandLine::whiteChromaOption, parseCieXy};
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

    CommandLine::toneMapName = toneMapNameOut;
    CommandLine::toneMapOptions = &toneMapOptionsContext;
    CommandLine::makeToneMappingMethodsString();
    OptionParser<OptionBase>::parse(argc, argv, toneMappingOptions, 12);
    ToneMap::recomputeGammaTables(toneMapOptionsContext, (*CommandLine::toneMapOptions).gamma);
    CommandLine::toneMapOptions = nullptr;
    CommandLine::toneMapName = nullptr;
}

void
CommandLine::radianceMethodParseOptions(
        int *argc,
        char **argv,
        char *radianceMethodsStringOut)
{
    CommandLine::radianceMethodsString = radianceMethodsStringOut;
    TypedOption<char *> radianceMethodOpt = {"-radiance-method", &CommandLine::radianceMethodsString, 1, nullptr, nullptr};
    OptionBase radianceOptions[] = {
        REGISTER_OPTION(char *, radianceMethodOpt, 4)
    };
    OptionParser<OptionBase>::parse(argc, argv, radianceOptions, 1);
}

void
CommandLine::flatOption(int & /*value*/) {
    renderOptionsState.smoothShading = false;
}

void
CommandLine::noCullingOption(int & /*value*/) {
    renderOptionsState.backfaceCulling = false;
}

void
CommandLine::outlinesOption(int & /*value*/) {
    renderOptionsState.drawOutlines = true;
}

void
CommandLine::traceOption(int & /*value*/) {
    renderOptionsState.trace = true;
}

void
CommandLine::renderParseOptions(
        int *argc,
        char **argv,
        RenderOptions *renderOptions)
{
    TypedOption<int> flatOpt = {"-flat-shading", &trueValue, 0, CommandLine::flatOption, nullptr};
    TypedOption<int> raycastOpt = {"-raycast", &trueValue, 0, CommandLine::traceOption, nullptr};
    TypedOption<int> noCullingOpt = {"-no-culling", &trueValue, 0, CommandLine::noCullingOption, nullptr};
    TypedOption<int> outlinesOpt = {"-outlines", &trueValue, 0, CommandLine::outlinesOption, nullptr};
    TypedOption<ColorRgb> outlineColorOpt = {"-outline-color", &outlineColor, 3, nullptr, parseColor3};
    OptionBase renderingOptions[] = {
        REGISTER_OPTION(int, flatOpt, 5),
        REGISTER_OPTION(int, raycastOpt, 5),
        REGISTER_OPTION(int, noCullingOpt, 5),
        REGISTER_OPTION(int, outlinesOpt, 5),
        REGISTER_OPTION(ColorRgb, outlineColorOpt, 10)
    };

    renderOptionsState = *renderOptions;

    OptionParser<OptionBase>::parse(argc, argv, renderingOptions, 5);

    *renderOptions = renderOptionsState;
    renderOptions->outlineColor.r = outlineColor.r;
    renderOptions->outlineColor.g = outlineColor.g;
    renderOptions->outlineColor.b = outlineColor.b;
}

void
CommandLine::binaryOutputOption(const char *& /*value*/) {
    batchOptionsState.exportBinary =
        batchOptionsState.binaryOutputFilename != nullptr
        && batchOptionsState.binaryOutputFilename[0] != '\0';
}

void
CommandLine::binaryInputOption(const char *& /*value*/) {
    batchOptionsState.importBinary =
        batchOptionsState.binaryInputFilename != nullptr
        && batchOptionsState.binaryInputFilename[0] != '\0';
}

void
CommandLine::batchParseOptions(
        int *argc,
        char **argv,
        BatchOptions *options)
{
    TypedOption<int> iterationsOpt = {"-iterations", &batchOptionsState.iterations, 1, nullptr, nullptr};
    TypedOption<const char *> obfOpt = {"-obf", &batchOptionsState.binaryOutputFilename, 1, CommandLine::binaryOutputOption, nullptr};
    TypedOption<const char *> ibfOpt = {"-ibf", &batchOptionsState.binaryInputFilename, 1, CommandLine::binaryInputOption, nullptr};
    TypedOption<const char *> radianceImageOpt = {"-radiance-image-savefile", &batchOptionsState.radianceImageFileNameFormat, 1, nullptr, nullptr};
    TypedOption<const char *> radianceModelOpt = {"-radiance-model-savefile", &batchOptionsState.radianceModelFileNameFormat, 1, nullptr, nullptr};
    TypedOption<int> saveModuloOpt = {"-save-modulo", &batchOptionsState.saveModulo, 1, nullptr, nullptr};
    TypedOption<const char *> raytracingImageOpt = {"-raytracing-image-savefile", &batchOptionsState.raytracingImageFileName, 1, nullptr, nullptr};
    TypedOption<int> timingsOpt = {"-timings", &batchOptionsState.timings, 0, setIntTrue, nullptr};
    OptionBase batchCommandLineOptions[] = {
        REGISTER_OPTION(int, iterationsOpt, 3),
        REGISTER_OPTION(const char *, obfOpt, 4),
        REGISTER_OPTION(const char *, ibfOpt, 4),
        REGISTER_OPTION(const char *, radianceImageOpt, 12),
        REGISTER_OPTION(const char *, radianceModelOpt, 12),
        REGISTER_OPTION(int, saveModuloOpt, 8),
        REGISTER_OPTION(const char *, raytracingImageOpt, 14),
        REGISTER_OPTION(int, timingsOpt, 3)
    };

    batchOptionsState = *options;
    batchOptionsState.exportBinary = false;
    batchOptionsState.importBinary = false;
    OptionParser<OptionBase>::parse(argc, argv, batchCommandLineOptions, 8);
    *options = batchOptionsState;
}

#ifdef RAYTRACING_ENABLED

EnumDesc CommandLine::approximateValues[] = {
    {StochasticRaytracingApproximation::CONSTANT, "constant", 2},
    {StochasticRaytracingApproximation::LINEAR, "linear", 2},
    {StochasticRaytracingApproximation::BI_LINEAR, "bilinear", 2},
    {StochasticRaytracingApproximation::QUADRATIC, "quadratic", 2},
    {StochasticRaytracingApproximation::CUBIC, "cubic", 2},
    {0, nullptr, 0}
};

EnumDesc CommandLine::clusteringValues[] = {
    {HierarchyClusteringMode::NO_CLUSTERING, "none", 2},
    {HierarchyClusteringMode::ISOTROPIC_CLUSTERING, "isotropic", 2},
    {HierarchyClusteringMode::ORIENTED_CLUSTERING, "oriented",  2},
    {0, nullptr, 0}
};

EnumDesc CommandLine::sequenceValues[] = {
    {Sampler4DSequence::RANDOM, "PseudoRandom", 2},
    {Sampler4DSequence::HALTON,"Halton", 2},
    {Sampler4DSequence::NIEDERREITER, "Niederreiter", 2}, // TODO: Not able to select all available sequences...
    {0, nullptr, 0}
};

EnumDesc CommandLine::estimatorTypeValues[] = {
    {RandomWalkEstimatorType::RW_SHOOTING, "Shooting", 2},
    {RandomWalkEstimatorType::RW_GATHERING, "Gathering", 2},
    {0, nullptr, 0}
};

EnumDesc CommandLine::estimatorKindValues[] = {
    {RandomWalkEstimatorKind::RW_COLLISION, "Collision", 2},
    {RandomWalkEstimatorKind::RW_ABSORPTION, "Absorption", 2},
    {RandomWalkEstimatorKind::RW_SURVIVAL, "Survival", 2},
    {RandomWalkEstimatorKind::RW_LAST_BUT_NTH, "Last-but-N", 2},
    {RandomWalkEstimatorKind::RW_N_LAST, "Last-N", 2},
    {0, nullptr, 0}
};

EnumDesc CommandLine::showWhatValues[] = {
    {WhatToShow::SHOW_TOTAL_RADIANCE, "total-radiance", 2},
    {WhatToShow::SHOW_INDIRECT_RADIANCE, "indirect-radiance", 2},
    {WhatToShow::SHOW_IMPORTANCE, "importance", 2},
    {0, nullptr, 0}
};

void
CommandLine::stochasticRelaxationRadiosityParseOptions(
        int *argc,
        char **argv,
        StochasticRelaxation &stochasticRelaxationState,
        ElementHierarchyState &elementHierarchyState)
{
    EnumBinding<Sampler4DSequence> sequenceBinding = {&stochasticRelaxationState.sequence, sequenceValues};
    EnumBinding<StochasticRaytracingApproximation> approximationBinding = {&stochasticRelaxationState.approximationOrderType, approximateValues};
    EnumBinding<HierarchyClusteringMode> clusteringBinding = {&elementHierarchyState.clustering, clusteringValues};
    EnumBinding<WhatToShow> showBinding = {&stochasticRelaxationState.show, showWhatValues};
    TypedOption<int> srrRayUnitsOpt = {"-srr-ray-units", &stochasticRelaxationState.rayUnitsPerIt, 1, nullptr, nullptr};
    TypedOption<int> srrBidirectionalOpt = {"-srr-bidirectional", &stochasticRelaxationState.bidirectionalTransfers, 1, nullptr, parseBoolInt};
    TypedOption<int> srrControlVariateOpt = {"-srr-control-variate", &stochasticRelaxationState.constantControlVariate, 1, nullptr, parseBoolInt};
    TypedOption<int> srrIndirectOnlyOpt = {"-srr-indirect-only", &stochasticRelaxationState.indirectOnly, 1, nullptr, parseBoolInt};
    TypedOption<int> srrImportanceDrivenOpt = {"-srr-importance-driven", &stochasticRelaxationState.importanceDriven, 1, nullptr, parseBoolInt};
    TypedOption<EnumBinding<Sampler4DSequence>> srrSequenceOpt = {"-srr-sampling-sequence", &sequenceBinding, 1, nullptr, parseEnumBinding<Sampler4DSequence>};
    TypedOption<EnumBinding<StochasticRaytracingApproximation>> srrApproximationOpt = {"-srr-approximation", &approximationBinding, 1, nullptr, parseEnumBinding<StochasticRaytracingApproximation>};
    TypedOption<int> srrHierarchicalOpt = {"-srr-hierarchical", &elementHierarchyState.do_h_meshing, 1, nullptr, parseBoolInt};
    TypedOption<EnumBinding<HierarchyClusteringMode>> srrClusteringOpt = {"-srr-clustering", &clusteringBinding, 1, nullptr, parseEnumBinding<HierarchyClusteringMode>};
    TypedOption<float> srrEpsilonOpt = {"-srr-epsilon", &elementHierarchyState.epsilon, 1, nullptr, nullptr};
    TypedOption<float> srrMinAreaOpt = {"-srr-minarea", &elementHierarchyState.minimumArea, 1, nullptr, nullptr};
    TypedOption<EnumBinding<WhatToShow>> srrDisplayOpt = {"-srr-display", &showBinding, 1, nullptr, parseEnumBinding<WhatToShow>};
    TypedOption<int> srrDiscardIncrementalOpt = {"-srr-discard-incremental", &stochasticRelaxationState.discardIncremental, 1, nullptr, parseBoolInt};
    TypedOption<int> srrIncrementalImportanceOpt = {"-srr-incremental-uses-importance", &stochasticRelaxationState.incrementalUsesImportance, 1, nullptr, parseBoolInt};
    TypedOption<int> srrNaiveMergingOpt = {"-srr-naive-merging", &stochasticRelaxationState.naiveMerging, 1, nullptr, parseBoolInt};
    TypedOption<int> srrNonDiffuseFirstShotOpt = {"-srr-nondiffuse-first-shot", &stochasticRelaxationState.doNonDiffuseFirstShot, 1, nullptr, parseBoolInt};
    TypedOption<int> srrInitialLsSamplesOpt = {"-srr-initial-ls-samples", &stochasticRelaxationState.initialLightSourceSamples, 1, nullptr, nullptr};
    OptionBase srrOptions[] = {
        REGISTER_OPTION(int, srrRayUnitsOpt, 8),
        REGISTER_OPTION(int, srrBidirectionalOpt, 7),
        REGISTER_OPTION(int, srrControlVariateOpt, 7),
        REGISTER_OPTION(int, srrIndirectOnlyOpt, 7),
        REGISTER_OPTION(int, srrImportanceDrivenOpt, 7),
        REGISTER_OPTION(EnumBinding<Sampler4DSequence>, srrSequenceOpt, 7),
        REGISTER_OPTION(EnumBinding<StochasticRaytracingApproximation>, srrApproximationOpt, 7),
        REGISTER_OPTION(int, srrHierarchicalOpt, 7),
        REGISTER_OPTION(EnumBinding<HierarchyClusteringMode>, srrClusteringOpt, 7),
        REGISTER_OPTION(float, srrEpsilonOpt, 7),
        REGISTER_OPTION(float, srrMinAreaOpt, 7),
        REGISTER_OPTION(EnumBinding<WhatToShow>, srrDisplayOpt, 7),
        REGISTER_OPTION(int, srrDiscardIncrementalOpt, 7),
        REGISTER_OPTION(int, srrIncrementalImportanceOpt, 7),
        REGISTER_OPTION(int, srrNaiveMergingOpt, 7),
        REGISTER_OPTION(int, srrNonDiffuseFirstShotOpt, 7),
        REGISTER_OPTION(int, srrInitialLsSamplesOpt, 7)
    };

    OptionParser<OptionBase>::parse(argc, argv, srrOptions, 17);
}

void
CommandLine::randomWalkRadiosityParseOptions(
        int *argc,
        char **argv,
        StochasticRelaxation &stochasticRelaxationState)
{
    EnumBinding<Sampler4DSequence> sequenceBinding = {&stochasticRelaxationState.sequence, sequenceValues};
    EnumBinding<StochasticRaytracingApproximation> approximationBinding = {&stochasticRelaxationState.approximationOrderType, approximateValues};
    EnumBinding<RandomWalkEstimatorType> estimatorTypeBinding = {&stochasticRelaxationState.randomWalkEstimatorType, estimatorTypeValues};
    EnumBinding<RandomWalkEstimatorKind> estimatorKindBinding = {&stochasticRelaxationState.randomWalkEstimatorKind, estimatorKindValues};
    TypedOption<int> rwrRayUnitsOpt = {"-rwr-ray-units", &stochasticRelaxationState.rayUnitsPerIt, 1, nullptr, nullptr};
    TypedOption<int> rwrContinuousOpt = {"-rwr-continuous", &stochasticRelaxationState.continuousRandomWalk, 1, nullptr, parseBoolInt};
    TypedOption<int> rwrControlVariateOpt = {"-rwr-control-variate", &stochasticRelaxationState.constantControlVariate, 1, nullptr, parseBoolInt};
    TypedOption<int> rwrIndirectOnlyOpt = {"-rwr-indirect-only", &stochasticRelaxationState.indirectOnly, 1, nullptr, parseBoolInt};
    TypedOption<EnumBinding<Sampler4DSequence>> rwrSequenceOpt = {"-rwr-sampling-sequence", &sequenceBinding, 1, nullptr, parseEnumBinding<Sampler4DSequence>};
    TypedOption<EnumBinding<StochasticRaytracingApproximation>> rwrApproximationOpt = {"-rwr-approximation", &approximationBinding, 1, nullptr, parseEnumBinding<StochasticRaytracingApproximation>};
    TypedOption<EnumBinding<RandomWalkEstimatorType>> rwrEstimatorOpt = {"-rwr-estimator", &estimatorTypeBinding, 1, nullptr, parseEnumBinding<RandomWalkEstimatorType>};
    TypedOption<EnumBinding<RandomWalkEstimatorKind>> rwrScoreOpt = {"-rwr-score", &estimatorKindBinding, 1, nullptr, parseEnumBinding<RandomWalkEstimatorKind>};
    TypedOption<int> rwrNumlastOpt = {"-rwr-numlast", &stochasticRelaxationState.randomWalkNumLast, 1, nullptr, nullptr};
    OptionBase rwrOptions[] = {
        REGISTER_OPTION(int, rwrRayUnitsOpt, 8),
        REGISTER_OPTION(int, rwrContinuousOpt, 7),
        REGISTER_OPTION(int, rwrControlVariateOpt, 7),
        REGISTER_OPTION(int, rwrIndirectOnlyOpt, 7),
        REGISTER_OPTION(EnumBinding<Sampler4DSequence>, rwrSequenceOpt, 7),
        REGISTER_OPTION(EnumBinding<StochasticRaytracingApproximation>, rwrApproximationOpt, 7),
        REGISTER_OPTION(EnumBinding<RandomWalkEstimatorType>, rwrEstimatorOpt, 7),
        REGISTER_OPTION(EnumBinding<RandomWalkEstimatorKind>, rwrScoreOpt, 7),
        REGISTER_OPTION(int, rwrNumlastOpt, 12)
    };

    OptionParser<OptionBase>::parse(argc, argv, rwrOptions, 9);
}

EnumDesc CommandLine::rayMatterPixelFilterValues[] = {
    {RayMatterFilterType::BOX_FILTER, "box", 2},
    {RayMatterFilterType::TENT_FILTER, "tent", 2},
    {RayMatterFilterType::GAUSS_FILTER, "gaussian 1/sqrt2", 2},
    {RayMatterFilterType::GAUSS2_FILTER, "gaussian 1/2", 2},
    {0, nullptr, 0}
};

void
CommandLine::rayMattingParseOptions(
        int *argc,
        char **argv,
        RayMatterState &rayMatterState)
{
    EnumBinding<RayMatterFilterType> pixelFilterBinding = {&rayMatterState.filter, rayMatterPixelFilterValues};
    TypedOption<int> rmSamplesOpt = {"-rm-samples-per-pixel", &rayMatterState.samplesPerPixel, 1, nullptr, nullptr};
    TypedOption<EnumBinding<RayMatterFilterType>> rmPixelFilterOpt = {"-rm-pixel-filter", &pixelFilterBinding, 1, nullptr, parseEnumBinding<RayMatterFilterType>};
    OptionBase rayMatterOptions[] = {
        REGISTER_OPTION(int, rmSamplesOpt, 6),
        REGISTER_OPTION(EnumBinding<RayMatterFilterType>, rmPixelFilterOpt, 7)
    };

    OptionParser<OptionBase>::parse(argc, argv, rayMatterOptions, 2);
}

/*** Enum Option types ***/

EnumDesc CommandLine::rayTracingRadianceModeValues[] = {
    {RayTracingRadMode::STORED_NONE, "none", 2},
    {RayTracingRadMode::STORED_DIRECT, "direct", 2},
    {RayTracingRadMode::STORED_INDIRECT, "indirect", 2},
    {RayTracingRadMode::STORED_PHOTON_MAP, "photonmap", 2},
    {0, nullptr, 0}
};


EnumDesc CommandLine::rayTracingLightModeValues[] = {
    {RayTracingLightMode::POWER_LIGHTS, "power", 2},
    {RayTracingLightMode::IMPORTANT_LIGHTS, "important", 2},
    {RayTracingLightMode::ALL_LIGHTS, "all", 2},
    {0, nullptr, 0}
};


EnumDesc CommandLine::rayTracingSamplingModeValues[] = {
    {RayTracingSamplingMode::BRDF_SAMPLING, "bsdf", 2},
    {RayTracingSamplingMode::CLASSICAL_SAMPLING, "classical", 2},
    {0, nullptr, 0}
};

void
CommandLine::stochasticRayTracerParseOptions(
        int *argc,
        char **argv,
        StochasticRayTracingState &stochasticRayTracingState)
{
    EnumBinding<RayTracingRadMode> radModeBinding = {&stochasticRayTracingState.radMode, rayTracingRadianceModeValues};
    EnumBinding<RayTracingLightMode> lightModeBinding = {&stochasticRayTracingState.lightMode, rayTracingLightModeValues};
    EnumBinding<RayTracingSamplingMode> samplingModeBinding = {&stochasticRayTracingState.reflectionSampling, rayTracingSamplingModeValues};
    TypedOption<int> rtsSamplesPerPixelOpt = {"-rts-samples-per-pixel", &stochasticRayTracingState.samplesPerPixel, 1, nullptr, nullptr};
    TypedOption<int> rtsNoProgressiveOpt = {"-rts-no-progressive", &stochasticRayTracingState.progressiveTracing, 0, setIntFalse, nullptr};
    TypedOption<EnumBinding<RayTracingRadMode>> rtsRadModeOpt = {"-rts-rad-mode", &radModeBinding, 1, nullptr, parseEnumBinding<RayTracingRadMode>};
    TypedOption<int> rtsNoLightSamplingOpt = {"-rts-no-lightsampling", &stochasticRayTracingState.nextEvent, 0, setIntFalse, nullptr};
    TypedOption<EnumBinding<RayTracingLightMode>> rtsLightModeOpt = {"-rts-l-mode", &lightModeBinding, 1, nullptr, parseEnumBinding<RayTracingLightMode>};
    TypedOption<int> rtsLightSamplesOpt = {"-rts-l-samples", &stochasticRayTracingState.nextEventSamples, 1, nullptr, nullptr};
    TypedOption<int> rtsScatterSamplesOpt = {"-rts-scatter-samples", &stochasticRayTracingState.scatterSamples, 1, nullptr, nullptr};
    TypedOption<int> rtsDoFdgOpt = {"-rts-do-fdg", &stochasticRayTracingState.differentFirstDG, 0, setIntTrue, nullptr};
    TypedOption<int> rtsFdgSamplesOpt = {"-rts-fdg-samples", &stochasticRayTracingState.firstDGSamples, 1, nullptr, nullptr};
    TypedOption<int> rtsSeparateSpecularOpt = {"-rts-separate-specular", &stochasticRayTracingState.separateSpecular, 0, setIntTrue, nullptr};
    TypedOption<EnumBinding<RayTracingSamplingMode>> rtsSamplingModeOpt = {"-rts-s-mode", &samplingModeBinding, 1, nullptr, parseEnumBinding<RayTracingSamplingMode>};
    TypedOption<int> rtsMinPathLengthOpt = {"-rts-min-path-length", &stochasticRayTracingState.minPathDepth, 1, nullptr, nullptr};
    TypedOption<int> rtsMaxPathLengthOpt = {"-rts-max-path-length", &stochasticRayTracingState.maxPathDepth, 1, nullptr, nullptr};
    TypedOption<int> rtsNoDirectBackgroundOpt = {"-rts-NOdirect-background-rad", &stochasticRayTracingState.backgroundDirect, 0, setIntFalse, nullptr};
    TypedOption<int> rtsNoIndirectBackgroundOpt = {"-rts-NOindirect-background-rad", &stochasticRayTracingState.backgroundIndirect, 0, setIntFalse, nullptr};
    OptionBase stochasticRatTracerOptions[] = {
        REGISTER_OPTION(int, rtsSamplesPerPixelOpt, 7),
        REGISTER_OPTION(int, rtsNoProgressiveOpt, 9),
        REGISTER_OPTION(EnumBinding<RayTracingRadMode>, rtsRadModeOpt, 8),
        REGISTER_OPTION(int, rtsNoLightSamplingOpt, 9),
        REGISTER_OPTION(EnumBinding<RayTracingLightMode>, rtsLightModeOpt, 8),
        REGISTER_OPTION(int, rtsLightSamplesOpt, 8),
        REGISTER_OPTION(int, rtsScatterSamplesOpt, 7),
        REGISTER_OPTION(int, rtsDoFdgOpt, 0),
        REGISTER_OPTION(int, rtsFdgSamplesOpt, 8),
        REGISTER_OPTION(int, rtsSeparateSpecularOpt, 8),
        REGISTER_OPTION(EnumBinding<RayTracingSamplingMode>, rtsSamplingModeOpt, 9),
        REGISTER_OPTION(int, rtsMinPathLengthOpt, 8),
        REGISTER_OPTION(int, rtsMaxPathLengthOpt, 8),
        REGISTER_OPTION(int, rtsNoDirectBackgroundOpt, 8),
        REGISTER_OPTION(int, rtsNoIndirectBackgroundOpt, 8)
    };

    OptionParser<OptionBase>::parse(argc, argv, stochasticRatTracerOptions, 15);
}

int CommandLine::regExpStringLength = BidirectionalPathRaytracerConfig::MAX_REGEXP_SIZE;

void
CommandLine::biDirectionalPathParseOptions(
        int *argc,
        char **argv,
        BidirectionalPathTracingState &bidirectionalPathState)
{
    FixedStringBinding leBinding = {bidirectionalPathState.baseConfig.leRegExp, regExpStringLength};
    FixedStringBinding ldBinding = {bidirectionalPathState.baseConfig.ldRegExp, regExpStringLength};
    FixedStringBinding liBinding = {bidirectionalPathState.baseConfig.liRegExp, regExpStringLength};
    TypedOption<int> bidirSamplesPerPixelOpt = {"-bidir-samples-per-pixel", &bidirectionalPathState.baseConfig.samplesPerPixel, 1, nullptr, nullptr};
    TypedOption<int> bidirNoProgressiveOpt = {"-bidir-no-progressive", &bidirectionalPathState.baseConfig.progressiveTracing, 0, setIntFalse, nullptr};
    TypedOption<int> bidirMaxEyePathLengthOpt = {"-bidir-max-eye-path-length", &bidirectionalPathState.baseConfig.maximumEyePathDepth, 1, nullptr, nullptr};
    TypedOption<int> bidirMaxLightPathLengthOpt = {"-bidir-max-light-path-length", &bidirectionalPathState.baseConfig.maximumLightPathDepth, 1, nullptr, nullptr};
    TypedOption<int> bidirMaxPathLengthOpt = {"-bidir-max-path-length", &bidirectionalPathState.baseConfig.maximumPathDepth, 1, nullptr, nullptr};
    TypedOption<int> bidirMinPathLengthOpt = {"-bidir-min-path-length", &bidirectionalPathState.baseConfig.minimumPathDepth, 1, nullptr, nullptr};
    TypedOption<int> bidirNoLightImportanceOpt = {"-bidir-no-light-importance", &bidirectionalPathState.baseConfig.sampleImportantLights, 0, setIntFalse, nullptr};
    TypedOption<int> bidirUseRegexpOpt = {"-bidir-use-regexp", &bidirectionalPathState.baseConfig.useSpars, 0, setIntTrue, nullptr};
    TypedOption<int> bidirUseEmittedOpt = {"-bidir-use-emitted", &bidirectionalPathState.baseConfig.doLe, 1, nullptr, parseBoolInt};
    TypedOption<FixedStringBinding> bidirRexpEmittedOpt = {"-bidir-rexp-emitted", &leBinding, 1, nullptr, parseFixedStringBinding};
    TypedOption<int> bidirRegDirectOpt = {"-bidir-reg-direct", &bidirectionalPathState.baseConfig.doLD, 1, nullptr, parseBoolInt};
    TypedOption<FixedStringBinding> bidirRexpDirectOpt = {"-bidir-rexp-direct", &ldBinding, 1, nullptr, parseFixedStringBinding};
    TypedOption<int> bidirRegIndirectOpt = {"-bidir-reg-indirect", &bidirectionalPathState.baseConfig.doLI, 1, nullptr, parseBoolInt};
    TypedOption<FixedStringBinding> bidirRexpIndirectOpt = {"-bidir-rexp-indirect", &liBinding, 1, nullptr, parseFixedStringBinding};
    OptionBase bidirectionalOptions[] = {
        REGISTER_OPTION(int, bidirSamplesPerPixelOpt, 8),
        REGISTER_OPTION(int, bidirNoProgressiveOpt, 11),
        REGISTER_OPTION(int, bidirMaxEyePathLengthOpt, 12),
        REGISTER_OPTION(int, bidirMaxLightPathLengthOpt, 12),
        REGISTER_OPTION(int, bidirMaxPathLengthOpt, 12),
        REGISTER_OPTION(int, bidirMinPathLengthOpt, 12),
        REGISTER_OPTION(int, bidirNoLightImportanceOpt, 11),
        REGISTER_OPTION(int, bidirUseRegexpOpt, 12),
        REGISTER_OPTION(int, bidirUseEmittedOpt, 12),
        REGISTER_OPTION(FixedStringBinding, bidirRexpEmittedOpt, 13),
        REGISTER_OPTION(int, bidirRegDirectOpt, 12),
        REGISTER_OPTION(FixedStringBinding, bidirRexpDirectOpt, 13),
        REGISTER_OPTION(int, bidirRegIndirectOpt, 12),
        REGISTER_OPTION(FixedStringBinding, bidirRexpIndirectOpt, 13)
    };

    OptionParser<OptionBase>::parse(argc, argv, bidirectionalOptions, 14);
}

char *CommandLine::raytracingMethodsString = nullptr;
char *CommandLine::rayTracerName = nullptr;

void
CommandLine::mainRayTracingOption(char *&name) {
    strcpy(rayTracerName, name);
}

void
CommandLine::rayTracingParseOptions(
        int *argc,
        char **argv,
        char raytracingMethodsStringOut[],
        char *rayTracerNameOut)
{
    char *raytracingMethodName = nullptr;
    TypedOption<char *> raytracingMethodOpt = {"-raytracing-method", &raytracingMethodName, 1, CommandLine::mainRayTracingOption, nullptr};
    OptionBase raytracingOptions[] = {
        REGISTER_OPTION(char *, raytracingMethodOpt, 4)
    };

    CommandLine::rayTracerName = rayTracerNameOut;
    CommandLine::raytracingMethodsString = raytracingMethodsStringOut;
    OptionParser<OptionBase>::parse(argc, argv, raytracingOptions, 1);
}

void
CommandLine::photonMapParseOptions(
        int *argc,
        char **argv,
        PhotonMapState &photonMapState)
{
    TypedOption<int> pmapDoGlobalOpt = {"-pmap-do-global", &photonMapState.doGlobalMap, 1, nullptr, parseBoolInt};
    TypedOption<long> pmapGlobalPathsOpt = {"-pmap-global-paths", &photonMapState.gPathsPerIteration, 1, nullptr, nullptr};
    TypedOption<int> pmapGPreirradianceOpt = {"-pmap-g-preirradiance", &photonMapState.precomputeGIrradiance, 1, nullptr, parseBoolInt};
    TypedOption<int> pmapDoCausticOpt = {"-pmap-do-caustic", &photonMapState.doCausticMap, 1, nullptr, parseBoolInt};
    TypedOption<long> pmapCausticPathsOpt = {"-pmap-caustic-paths", &photonMapState.cPathsPerIteration, 1, nullptr, nullptr};
    TypedOption<int> pmapRenderHitsOpt = {"-pmap-render-hits", &photonMapState.renderImage, 0, setIntTrue, nullptr};
    TypedOption<int> pmapReconGPhotonsOpt = {"-pmap-recon-gphotons", &photonMapState.reconGPhotons, 1, nullptr, nullptr};
    TypedOption<int> pmapReconIPhotonsOpt = {"-pmap-recon-iphotons", &photonMapState.reconCPhotons, 1, nullptr, nullptr};
    TypedOption<int> pmapReconPhotonsOpt = {"-pmap-recon-photons", &photonMapState.reconIPhotons, 1, nullptr, nullptr};
    TypedOption<int> pmapBalancingOpt = {"-pmap-balancing", &photonMapState.balanceKDTree, 1, nullptr, parseBoolInt};
    OptionBase photonMapOptions[] = {
        REGISTER_OPTION(int, pmapDoGlobalOpt, 9),
        REGISTER_OPTION(long, pmapGlobalPathsOpt, 9),
        REGISTER_OPTION(int, pmapGPreirradianceOpt, 11),
        REGISTER_OPTION(int, pmapDoCausticOpt, 9),
        REGISTER_OPTION(long, pmapCausticPathsOpt, 9),
        REGISTER_OPTION(int, pmapRenderHitsOpt, 9),
        REGISTER_OPTION(int, pmapReconGPhotonsOpt, 9),
        REGISTER_OPTION(int, pmapReconIPhotonsOpt, 9),
        REGISTER_OPTION(int, pmapReconPhotonsOpt, 9),
        REGISTER_OPTION(int, pmapBalancingOpt, 9)
    };
    OptionParser<OptionBase>::parse(argc, argv, photonMapOptions, 10);
}

#endif
