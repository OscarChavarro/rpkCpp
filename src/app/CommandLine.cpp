#include <cstdlib>
#include <cstring>

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
    #include "photonMap/PhotonMapState.h"
#endif

#include "app/BackgroundMode.h"
#include "app/Options.h"
#include "app/OptionsType.h"
#include "app/BatchOptions.h"
#include "app/CommandLine.h"

// Default scene level configuration
static constexpr int DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS = 4;
static constexpr bool DEFAULT_FORCE_ONE_SIDED = true;

// Default virtual camera
static const Vector3D DEFAULT_CAMERA_EYE_POSITION(10.0, 0.0, 0.0);
static const Vector3D DEFAULT_CAMERA_LOOK_POSITION(0.0, 0.0, 0.0);
static const Vector3D DEFAULT_CAMERA_UP_DIRECTION(0.0, 0.0, 1.0);
static const ColorRgb DEFAULT_BACKGROUND_COLOR(0.0, 0.0, 0.0);
static constexpr float DEFAULT_CAMERA_FIELD_OF_VIEW = 22.5f;
static int globalNumberOfQuarterCircleDivisions = DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
static int globalFileOptionsForceOneSidedSurfaces = 0;
static int globalYes = 1;
static int globalNo = 0;
static int globalOutputImageWidth = 1920;
static int globalOutputImageHeight = 1080;
static int globalGlutDebugEnabled = false;
static Camera globalCamera;
static BackgroundMode globalBackgroundMode = BackgroundMode::NONE;
static ColorRgb globalBackgroundColor = DEFAULT_BACKGROUND_COLOR;

ColorRgb
CommandLine::commandLineDefaultBackgroundColor() {
    return DEFAULT_BACKGROUND_COLOR;
}

Background *
CommandLine::commandLineCreateBackground() {
    if ( globalBackgroundMode == BackgroundMode::SOLID ) {
        return new ConstantColorBackground(globalBackgroundColor);
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
            globalBackgroundMode = BackgroundMode::SOLID;
            globalBackgroundColor = parsedColor;
        }
        readIndex += 5;
    }

    while ( writeIndex < *argc ) {
        argv[writeIndex++] = nullptr;
    }
    *argc = writeIndex;
}

void
CommandLine::mainForceOneSidedOption(void *value) {
    globalFileOptionsForceOneSidedSurfaces = *static_cast<int *>(value);
}

void
CommandLine::mainMonochromeOption(void *value) {
    globalNumberOfQuarterCircleDivisions = *static_cast<int *>(value);
}

void
CommandLine::commandLineImageWidthOption(void *value) {
    globalOutputImageWidth = *static_cast<int *>(value);
}

void
CommandLine::commandLineImageHeightOption(void *value) {
    globalOutputImageHeight = *static_cast<int *>(value);
}

void
CommandLine::commandLineGeneralProgramParseOptions(
        int *argc,
        char **argv,
        bool *oneSidedSurfaces,
        int *conicSubDivisions,
        int *imageOutputWidth,
        int *imageOutputHeight,
        bool *glutDebugEnabled,
        OptionsType &optionTypes)
{
    CommandLineOptionDescription globalOptions[] = {
        {"-nqcdivs", 3, &optionTypes.intType, &globalNumberOfQuarterCircleDivisions, DEFAULT_ACTION,
         "-nqcdivs <integer>\t: number of quarter circle divisions"},
        {"-force-onesided", 10, nullptr, &globalYes, CommandLine::mainForceOneSidedOption,
         "-force-onesided\t\t: force one-sided surfaces"},
        {"-dont-force-onesided", 14, nullptr, &globalNo, CommandLine::mainForceOneSidedOption,
         "-dont-force-onesided\t: allow two-sided surfaces"},
        {"-monochromatic", 5, nullptr, &globalYes, CommandLine::mainMonochromeOption,
         "-monochromatic \t\t: convert colors to shades of grey"},
        {"-width", 5, &optionTypes.intType, &globalOutputImageWidth, CommandLine::commandLineImageWidthOption,
                "-width \t\t: image output width in pixels"},
        {"-height", 6, &optionTypes.intType, &globalOutputImageHeight, CommandLine::commandLineImageHeightOption,
                "-width \t\t: image output width in pixels"},
        {"-glutDebug", 6, &optionTypes.setTrueType, &globalGlutDebugEnabled, DEFAULT_ACTION,
                "-glutDebug\t\t: open interactive GLUT debug window after rendering"},
        {nullptr, 0, nullptr, nullptr, DEFAULT_ACTION, nullptr}
    };

    globalFileOptionsForceOneSidedSurfaces = DEFAULT_FORCE_ONE_SIDED;
    globalNumberOfQuarterCircleDivisions = DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
    globalBackgroundMode = BackgroundMode::NONE;
    globalBackgroundColor = DEFAULT_BACKGROUND_COLOR;
    globalGlutDebugEnabled = false;
    CommandLine::commandLineParseBackgroundOption(argc, argv);
    Options::parseGeneralOptions(globalOptions, argc, argv); // Order is important, this should be called last

    if ( globalFileOptionsForceOneSidedSurfaces != 0 ) {
        *oneSidedSurfaces = true;
    } else {
        *oneSidedSurfaces = false;
    }
    *conicSubDivisions = globalNumberOfQuarterCircleDivisions;
    *imageOutputWidth = globalOutputImageWidth;
    *imageOutputHeight = globalOutputImageHeight;
    *glutDebugEnabled = globalGlutDebugEnabled;

#ifndef OPEN_GL_ENABLED
    if ( globalGlutDebugEnabled ) {
        java::System::err.printf(
            "ERROR: Option '-glutDebug' requires OpenGL support. Recompile with -DOPEN_GL_ENABLED=ON.\n");
        java::System::err.flush();
        java::System::exit(1);
    }
#endif
}

void
CommandLine::cameraSetEyePositionOption(void *val) {
    const Vector3D *v = static_cast<Vector3D *>(val);
    globalCamera.setEyePosition(v->x, v->y, v->z);
}

void
CommandLine::cameraSetLookPositionOption(void *val) {
    const Vector3D *v = static_cast<Vector3D *>(val);
    globalCamera.setLookPosition(v->x, v->y, v->z);
}

void
CommandLine::cameraSetUpDirectionOption(void *val) {
    const Vector3D *v = static_cast<Vector3D *>(val);
    globalCamera.setUpDirection(v->x, v->y, v->z);
}

void
CommandLine::cameraSetFieldOfViewOption(void *val) {
    const float *v = static_cast<float *>(val);
    globalCamera.setFieldOfView(*v);
}

void
CommandLine::cameraDefaults(Camera *camera, int imageWidth, int imageHeight) {
    Vector3D eyePosition = DEFAULT_CAMERA_EYE_POSITION;
    Vector3D lookPosition = DEFAULT_CAMERA_LOOK_POSITION;
    Vector3D upDirection = DEFAULT_CAMERA_UP_DIRECTION;
    ColorRgb backgroundColor = DEFAULT_BACKGROUND_COLOR;

    camera->set(
        &eyePosition,
        &lookPosition,
        &upDirection,
        DEFAULT_CAMERA_FIELD_OF_VIEW,
        imageWidth,
        imageHeight,
        &backgroundColor);
}

void
CommandLine::cameraParseOptions(
        int *argc,
        char **argv,
        Camera *camera,
        int imageWidth,
        int imageHeight,
        OptionsType &optionTypes)
{
    CommandLineOptionDescription globalCameraOptions[] = {
        {"-eyepoint", 4, &optionTypes.vectorType, &globalCamera.eyePosition, CommandLine::cameraSetEyePositionOption,
         "-eyepoint  <vector>\t: viewing position"},
        {"-center", 4, &optionTypes.vectorType, &globalCamera.lookPosition, CommandLine::cameraSetLookPositionOption,
         "-center    <vector>\t: point looked at"},
        {"-updir", 3, &optionTypes.vectorType, &globalCamera.upDirection, CommandLine::cameraSetUpDirectionOption,
         "-updir     <vector>\t: direction pointing up"},
        {"-fov", 4, &optionTypes.floatType,  &globalCamera.fieldOfVision, CommandLine::cameraSetFieldOfViewOption,
         "-fov       <float> \t: field of view angle"},
        {nullptr, 0, nullptr, nullptr, nullptr, nullptr}
    };

    CommandLine::cameraDefaults(&globalCamera, imageWidth, imageHeight);
    Options::parseGeneralOptions(globalCameraOptions, argc, argv);
    *camera = globalCamera;
}

// Used for option management
static int globalTrue = true;
static int globalFalse = false;

void
CommandLine::iterationMethodOption(void *value) {
    char *name = *static_cast<char **>(value);

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
CommandLine::hierarchicalOption(void *value) {
    int yesno = *static_cast<int *>(value);

    if ( yesno != 0 ) {
        GalerkinRadianceMethod::galerkinState.hierarchical = true;
    } else {
        GalerkinRadianceMethod::galerkinState.hierarchical = false;
    }
}

void
CommandLine::lazyOption(void *value) {
    int yesno = *static_cast<int *>(value);
    GalerkinRadianceMethod::galerkinState.lazyLinking = yesno;
}

void
CommandLine::clusteringOption(void *value) {
    int yesno = *static_cast<int *>(value);
    GalerkinRadianceMethod::galerkinState.clustered = yesno;
}

void
CommandLine::importanceOption(void *value) {
    int yesno = *static_cast<int *>(value);
    GalerkinRadianceMethod::galerkinState.importanceDriven = yesno;
}

void
CommandLine::ambientOption(void *value) {
    int yesno = *static_cast<int *>(value);
    GalerkinRadianceMethod::galerkinState.useAmbientRadiance = yesno;
}

void
CommandLine::galerkinParseOptions(int *argc, char **argv, OptionsType &optionTypes) {
    CommandLineOptionDescription galerkinOptions[] = {
        {"-gr-iteration-method", 6, &optionTypes.stringType, nullptr, CommandLine::iterationMethodOption,
        "-gr-iteration-method <methodname>: Jacobi, GaussSeidel, Southwell"},
        {"-gr-hierarchical", 6, nullptr, static_cast<void *>(&globalTrue), CommandLine::hierarchicalOption,
        "-gr-hierarchical    \t: do hierarchical refinement"},
        {"-gr-not-hierarchical", 10, nullptr, static_cast<void *>(&globalFalse), CommandLine::hierarchicalOption,
        "-gr-not-hierarchical\t: don't do hierarchical refinement"},
        {"-gr-lazy-linking", 6, nullptr, static_cast<void *>(&globalTrue), CommandLine::lazyOption,
        "-gr-lazy-linking    \t: do lazy linking"},
        {"-gr-no-lazy-linking", 10, nullptr, static_cast<void *>(&globalFalse), CommandLine::lazyOption,
        "-gr-no-lazy-linking \t: don't do lazy linking"},
        {"-gr-clustering", 6, nullptr, static_cast<void *>(&globalTrue), CommandLine::clusteringOption,
        "-gr-clustering      \t: do clustering"},
        {"-gr-no-clustering", 10, nullptr, static_cast<void *>(&globalFalse), CommandLine::clusteringOption,
        "-gr-no-clustering   \t: don't do clustering"},
        {"-gr-importance", 6, nullptr, static_cast<void *>(&globalTrue), CommandLine::importanceOption,
        "-gr-importance      \t: do view-potential driven computations"},
        {"-gr-no-importance", 10, nullptr, static_cast<void *>(&globalFalse), CommandLine::importanceOption,
        "-gr-no-importance   \t: don't use view-potential"},
        {"-gr-ambient", 6, nullptr, static_cast<void *>(&globalTrue), CommandLine::ambientOption,
        "-gr-ambient         \t: do visualisation with ambient term"},
        {"-gr-no-ambient", 10, nullptr, static_cast<void *>(&globalFalse), CommandLine::ambientOption,
        "-gr-no-ambient      \t: do visualisation without ambient term"},
        {"-gr-link-error-threshold", 6, &optionTypes.floatType, &GalerkinRadianceMethod::galerkinState.relLinkErrorThreshold, nullptr,
        "-gr-link-error-threshold <float>: Relative link error threshold"},
        {"-gr-min-elem-area", 6, &optionTypes.floatType, &GalerkinRadianceMethod::galerkinState.relMinElemArea, nullptr,
        "-gr-min-elem-area <float> \t: Relative element area threshold"},
        {nullptr, 0, nullptr, nullptr, nullptr, nullptr}
    };

    Options::parseGeneralOptions(galerkinOptions, argc, argv);
}

// Composes explanation for -tonemapping command line option
static constexpr int STRING_LENGTH = 1000;
static char globalToneMappingMethodsString[STRING_LENGTH];
static float globalRxy[2];
static float globalGxy[2];
static float globalBxy[2];
static float globalWxy[2];

 void
CommandLine::makeToneMappingMethodsString() {
    strcpy(globalToneMappingMethodsString,
       "-tonemapping <method>: Set tone mapping method\n"
       "\tmethods: Lightness            Lightness Mapping (default)\n"
       "\t         TumblinRushmeier     Tumblin/Rushmeier's Mapping\n"
       "\t         Ward                 Ward's Mapping\n"
       "\t         RevisedTR            Revised Tumblin/Rushmeier's Mapping\n"
       "\t         Ferwerda             Partial Ferwerda's Mapping");
}

static char *globalToneMapName;
static ToneMappingContext *globalToneMapOptions = nullptr;

void
CommandLine::toneMappingMethodOption(void *value) {
    char *name = *static_cast<char **>(value);

    strcpy(globalToneMapName, name);
}

void
CommandLine::brightnessAdjustOption(void * /*val*/) {
    if ( globalToneMapOptions == nullptr ) {
        Error::fatal(-1, "CommandLine::brightnessAdjustOption", "ToneMappingContext not set");
    }
    (*globalToneMapOptions).pow_bright_adjust = java::Math::pow(2.0f, (*globalToneMapOptions).brightness_adjust);
}

void
CommandLine::chromaOption(void *value) {
    if ( globalToneMapOptions == nullptr ) {
        Error::fatal(-1, "CommandLine::chromaOption", "ToneMappingContext not set");
    }
    const float *chroma = static_cast<float *>(value);
    if ( chroma == globalRxy ) {
        (*globalToneMapOptions).xr = chroma[0];
        (*globalToneMapOptions).yr = chroma[1];
    } else if ( chroma == globalGxy ) {
        (*globalToneMapOptions).xg = chroma[0];
        (*globalToneMapOptions).yg = chroma[1];
    } else if ( chroma == globalBxy ) {
        (*globalToneMapOptions).xb = chroma[0];
        (*globalToneMapOptions).yb = chroma[1];
    } else if ( chroma == globalWxy ) {
        (*globalToneMapOptions).xw = chroma[0];
        (*globalToneMapOptions).yw = chroma[1];
    } else {
        Error::fatal(-1, "CommandLine::chromaOption", "invalid value pointer");
    }

    Cie::computeColorConversionTransforms(
        (*globalToneMapOptions).xr, (*globalToneMapOptions).yr,
        (*globalToneMapOptions).xg, (*globalToneMapOptions).yg,
        (*globalToneMapOptions).xb, (*globalToneMapOptions).yb,
        (*globalToneMapOptions).xw, (*globalToneMapOptions).yw);
}

void
CommandLine::toneMappingCommandLineOptionDescAdaptMethodOption(void *value) {
    if ( globalToneMapOptions == nullptr ) {
        Error::fatal(-1, "CommandLine::toneMappingCommandLineOptionDescAdaptMethodOption", "ToneMappingContext not set");
    }
    char *name = *static_cast<char **>(value);

    if ( strncasecmp(name, "average", 2) == 0 ) {
        (*globalToneMapOptions).staticAdaptationMethod = ToneMapAdaptationMethod::TMA_AVERAGE;
    } else if ( strncasecmp(name, "median", 2) == 0 ) {
        (*globalToneMapOptions).staticAdaptationMethod = ToneMapAdaptationMethod::TMA_MEDIAN;
    } else {
        Error::error(nullptr, "Invalid adaptation estimate method '%s'", name);
    }
}

void
CommandLine::gammaOption(void *value) {
    if ( globalToneMapOptions == nullptr ) {
        Error::fatal(-1, "CommandLine::gammaOption", "ToneMappingContext not set");
    }
    float gam = *static_cast<float *>(value);
    (*globalToneMapOptions).gamma.set(gam, gam, gam);
}

void
CommandLine::toneMapParseOptions(
        int *argc,
        char **argv,
        char *toneMapName,
        ToneMappingContext &toneMapOptions,
        OptionsType &optionTypes)
{
    CommandLineOptionDescription globalToneMappingOptions[] = {
        {"-tonemapping", 4, &optionTypes.stringType, nullptr, CommandLine::toneMappingMethodOption, globalToneMappingMethodsString},
        {"-brightness-adjust", 4, &optionTypes.floatType, nullptr, CommandLine::brightnessAdjustOption,
         "-brightness-adjust <float> : brightness adjustment factor"},
        {"-adapt", 5, &optionTypes.stringType, nullptr, CommandLine::toneMappingCommandLineOptionDescAdaptMethodOption,
        "-adapt <method>  \t: adaptation estimation method\n\tmethods: \"average\", \"median\""},
        {"-lwa", 3, &optionTypes.floatType, nullptr, DEFAULT_ACTION,
         "-lwa <float>\t\t: real world adaptation luminance"},
        {"-ldmax", 5, &optionTypes.floatType, nullptr, DEFAULT_ACTION,
         "-ldmax <float>\t\t: maximum diaply luminance"},
        {"-cmax", 4, &optionTypes.floatType, nullptr, DEFAULT_ACTION,
         "-cmax <float>\t\t: maximum displayable contrast"},
        {"-gamma", 4, &optionTypes.floatType, nullptr, CommandLine::gammaOption,
         "-gamma <float>       \t: gamma correction factor (same for red, green. blue)"},
        {"-rgbgamma", 4, &optionTypes.rgbType, nullptr, DEFAULT_ACTION,
         "-rgbgamma <r> <g> <b>\t: gamma correction factor (separate for red, green, blue)"},
        {"-red", 4, &optionTypes.xyType, globalRxy, CommandLine::chromaOption,
         "-red <xy>            \t: CIE xy chromaticity of monitor red"},
        {"-green", 4, &optionTypes.xyType, globalGxy, CommandLine::chromaOption,
         "-green <xy>          \t: CIE xy chromaticity of monitor green"},
        {"-blue", 4, &optionTypes.xyType, globalBxy, CommandLine::chromaOption,
         "-blue <xy>           \t: CIE xy chromaticity of monitor blue"},
        {"-white", 4, &optionTypes.xyType, globalWxy, CommandLine::chromaOption,
         "-white <xy>          \t: CIE xy chromaticity of monitor white"},
        {nullptr, 0, nullptr, nullptr, DEFAULT_ACTION, nullptr}
    };

    globalToneMapName = toneMapName;
    globalToneMapOptions = &toneMapOptions;
    globalToneMappingOptions[1].value = &(*globalToneMapOptions).brightness_adjust;
    globalToneMappingOptions[3].value = &(*globalToneMapOptions).realWorldAdaptionLuminance;
    globalToneMappingOptions[4].value = &(*globalToneMapOptions).maximumDisplayLuminance;
    globalToneMappingOptions[5].value = &(*globalToneMapOptions).maximumDisplayContrast;
    globalToneMappingOptions[7].value = &(*globalToneMapOptions).gamma;
    CommandLine::makeToneMappingMethodsString();
    Options::parseGeneralOptions(globalToneMappingOptions, argc, argv);
    ToneMap::recomputeGammaTables(toneMapOptions, (*globalToneMapOptions).gamma);
    globalToneMapOptions = nullptr;
    globalToneMapName = nullptr;
}

static char *globalRadianceMethodsString;

void
CommandLine::radianceMethodParseOptions(
        int *argc,
        char **argv,
        char *radianceMethodsString,
        OptionsType &optionTypes)
{
    CommandLineOptionDescription globalRadianceOptions[] = {
        {"-radiance-method", 4, &optionTypes.stringType,  nullptr, DEFAULT_ACTION, globalRadianceMethodsString},
        {nullptr, 0, nullptr, nullptr, DEFAULT_ACTION, nullptr}
    };

    globalRadianceMethodsString = radianceMethodsString;
    Options::parseGeneralOptions(globalRadianceOptions, argc, argv);
}

static RenderOptions globalRenderOptions;
static ColorRgb globalOutlineColor;

void
CommandLine::flatOption(void * /*value*/) {
    globalRenderOptions.smoothShading = false;
}

void
CommandLine::noCullingOption(void * /*value*/) {
    globalRenderOptions.backfaceCulling = false;
}

void
CommandLine::outlinesOption(void * /*value*/) {
    globalRenderOptions.drawOutlines = true;
}

void
CommandLine::traceOption(void * /*value*/) {
    globalRenderOptions.trace = true;
}

void
CommandLine::renderParseOptions(
        int *argc,
        char **argv,
        RenderOptions *renderOptions,
        OptionsType &optionTypes)
{
    CommandLineOptionDescription renderingOptions[] = {
        {"-flat-shading", 5, nullptr, nullptr, CommandLine::flatOption,
        "-flat-shading\t\t: render without Gouraud (color) interpolation"},
        {"-raycast", 5, nullptr, nullptr, CommandLine::traceOption,
        "-raycast\t\t: save raycasted scene view as a high dynamic range image"},
        {"-no-culling", 5, nullptr, nullptr, CommandLine::noCullingOption,
        "-no-culling\t\t: don't use backface culling"},
        {"-outlines", 5, nullptr, nullptr, CommandLine::outlinesOption,
        "-outlines\t\t: draw polygon outlines"},
        {"-outline-color", 10, &optionTypes.rgbType, &globalOutlineColor, DEFAULT_ACTION,
        "-outline-color <rgb> \t: color for polygon outlines"},
        {nullptr, 0, nullptr, nullptr, nullptr, nullptr}
    };

    globalRenderOptions = *renderOptions;

    Options::parseGeneralOptions(renderingOptions, argc, argv);

    *renderOptions = globalRenderOptions;
    renderOptions->outlineColor.r = globalOutlineColor.r;
    renderOptions->outlineColor.g = globalOutlineColor.g;
    renderOptions->outlineColor.b = globalOutlineColor.b;
}

static BatchOptions globalBatchOptions;

void
CommandLine::binaryOutputOption(void * /*value*/) {
    globalBatchOptions.exportBinary =
        globalBatchOptions.binaryOutputFilename != nullptr
        && globalBatchOptions.binaryOutputFilename[0] != '\0';
}

void
CommandLine::binaryInputOption(void * /*value*/) {
    globalBatchOptions.importBinary =
        globalBatchOptions.binaryInputFilename != nullptr
        && globalBatchOptions.binaryInputFilename[0] != '\0';
}

void
CommandLine::batchParseOptions(
        int *argc,
        char **argv,
        BatchOptions *options,
        OptionsType &optionTypes)
{
    CommandLineOptionDescription globalCommandLineBatchOptions[] = {
        {"-iterations", 3, &optionTypes.intType, &globalBatchOptions.iterations, DEFAULT_ACTION,
        "-iterations <integer>\t: world-space radiance iterations"},
        {"-obf", 4, &optionTypes.stringType, &globalBatchOptions.binaryOutputFilename, CommandLine::binaryOutputOption,
         "-obf <output.bin>\t: export loaded PersistedSceneModel snapshot to binary file"},
        {"-ibf", 4, &optionTypes.stringType, &globalBatchOptions.binaryInputFilename, CommandLine::binaryInputOption,
         "-ibf <input.bin>\t: import PersistedSceneModel snapshot from binary file (skips MGF read)"},
        {"-radiance-image-savefile", 12, &optionTypes.stringType, &globalBatchOptions.radianceImageFileNameFormat, DEFAULT_ACTION,
         "-radiance-image-savefile <filename>\t: radiance PPM/LOGLUV savefile name,\n\tfirst '%%d' will be substituted by iteration number"},
        {"-radiance-model-savefile", 12, &optionTypes.stringType, &globalBatchOptions.radianceModelFileNameFormat, DEFAULT_ACTION,
         "-radiance-model-savefile <filename>\t: radiance VRML model savefile name,"
         "\n\tfirst '%%d' will be substituted by iteration number"},
        {"-save-modulo", 8, &optionTypes.intType, &globalBatchOptions.saveModulo, DEFAULT_ACTION,
         "-save-modulo <integer>\t: save every n-th iteration"},
        {"-raytracing-image-savefile", 14, &optionTypes.stringType, &globalBatchOptions.raytracingImageFileName, DEFAULT_ACTION,
         "-raytracing-image-savefile <filename>\t: raytracing PPM savefile name"},
        {"-timings", 3, &optionTypes.setTrueType, &globalBatchOptions.timings, DEFAULT_ACTION,
         "-timings\t: printRegularHierarchy timings for world-space radiance and raytracing methods"},
        {nullptr, 0,  nullptr, nullptr, DEFAULT_ACTION, nullptr}
    };

    globalBatchOptions = *options;
    globalBatchOptions.exportBinary = false;
    globalBatchOptions.importBinary = false;
    Options::parseGeneralOptions(globalCommandLineBatchOptions, argc, argv);
    *options = globalBatchOptions;
}

#ifdef RAYTRACING_ENABLED

static EnumDesc globalApproximateValues[] = {
    {StochasticRaytracingApproximation::CONSTANT, "constant", 2},
    {StochasticRaytracingApproximation::LINEAR, "linear", 2},
    {StochasticRaytracingApproximation::BI_LINEAR, "bilinear", 2},
    {StochasticRaytracingApproximation::QUADRATIC, "quadratic", 2},
    {StochasticRaytracingApproximation::CUBIC, "cubic", 2},
    {0, nullptr, 0}
};
static CommandLineOptions approxTypeStruct = Options::makeEnumOptTypeStruct(globalApproximateValues);

static EnumDesc clusteringVals[] = {
    {HierarchyClusteringMode::NO_CLUSTERING, "none", 2},
    {HierarchyClusteringMode::ISOTROPIC_CLUSTERING, "isotropic", 2},
    {HierarchyClusteringMode::ORIENTED_CLUSTERING, "oriented",  2},
    {0, nullptr, 0}
};
static CommandLineOptions clusteringTypeStruct = Options::makeEnumOptTypeStruct(clusteringVals);

static EnumDesc sequenceVals[] = {
    {Sampler4DSequence::RANDOM, "PseudoRandom", 2},
    {Sampler4DSequence::HALTON,"Halton", 2},
    {Sampler4DSequence::NIEDERREITER, "Niederreiter", 2}, // TODO: Not able to select all available sequences...
    {0, nullptr, 0}
};
static CommandLineOptions sequenceTypeStruct = Options::makeEnumOptTypeStruct(sequenceVals);

static EnumDesc estTypeVals[] = {
    {RandomWalkEstimatorType::RW_SHOOTING, "Shooting", 2},
    {RandomWalkEstimatorType::RW_GATHERING, "Gathering", 2},
    {0, nullptr, 0}
};
static CommandLineOptions estTypeTypeStruct = Options::makeEnumOptTypeStruct(estTypeVals);

static EnumDesc globalEstKindValues[] = {
    {RandomWalkEstimatorKind::RW_COLLISION, "Collision", 2},
    {RandomWalkEstimatorKind::RW_ABSORPTION, "Absorption", 2},
    {RandomWalkEstimatorKind::RW_SURVIVAL, "Survival", 2},
    {RandomWalkEstimatorKind::RW_LAST_BUT_NTH, "Last-but-N", 2},
    {RandomWalkEstimatorKind::RW_N_LAST, "Last-N", 2},
    {0, nullptr, 0}
};
static CommandLineOptions estKindTypeStruct = Options::makeEnumOptTypeStruct(globalEstKindValues);

static EnumDesc showWhatVals[] = {
    {WhatToShow::SHOW_TOTAL_RADIANCE, "total-radiance", 2},
    {WhatToShow::SHOW_INDIRECT_RADIANCE, "indirect-radiance", 2},
    {WhatToShow::SHOW_IMPORTANCE, "importance", 2},
    {0, nullptr, 0}
};
static CommandLineOptions showWhatTypeStruct = Options::makeEnumOptTypeStruct(showWhatVals);

void
CommandLine::stochasticRelaxationRadiosityParseOptions(
        int *argc,
        char **argv,
        StochasticRelaxation &stochasticRelaxationState,
        ElementHierarchyState &elementHierarchyState,
        OptionsType &optionTypes)
{
    CommandLineOptionDescription srrOptions[] = {
        {"-srr-ray-units", 8, &optionTypes.intType, &stochasticRelaxationState.rayUnitsPerIt, DEFAULT_ACTION,
         "-srr-ray-units <n>          : To tune the amount of work in a single iteration"},
        {"-srr-bidirectional", 7, &optionTypes.boolType, &stochasticRelaxationState.bidirectionalTransfers, DEFAULT_ACTION,
         "-srr-bidirectional <yes|no> : Use lines bidirectionally"},
        {"-srr-control-variate", 7, &optionTypes.boolType, &stochasticRelaxationState.constantControlVariate, DEFAULT_ACTION,
         "-srr-control-variate <y|n>  : Constant Control Variate variance reduction"},
        {"-srr-indirect-only", 7, &optionTypes.boolType, &stochasticRelaxationState.indirectOnly, DEFAULT_ACTION,
         "-srr-indirect-only <y|n>    : Compute indirect illumination only"},
        {"-srr-importance-driven", 7, &optionTypes.boolType, &stochasticRelaxationState.importanceDriven, DEFAULT_ACTION,
         "-srr-importance-driven <y|n>: Use view-importance"},
        {"-srr-sampling-sequence", 7, &sequenceTypeStruct, &stochasticRelaxationState.sequence, DEFAULT_ACTION,
         "-srr-sampling-sequence <type>: \"PseudoRandom\", \"Niederreiter\""},
        {"-srr-approximation", 7, &approxTypeStruct, &stochasticRelaxationState.approximationOrderType, DEFAULT_ACTION,
         "-srr-approximation <order>  : \"constant\", \"linear\", \"quadratic\", \"cubic\""},
        {"-srr-hierarchical", 7, &optionTypes.boolType, &elementHierarchyState.do_h_meshing, DEFAULT_ACTION,
         "-srr-hierarchical <y|n>     : hierarchical refinement"},
        {"-srr-clustering", 7, &clusteringTypeStruct, &elementHierarchyState.clustering, DEFAULT_ACTION,
         "-srr-clustering <mode>      : \"none\", \"isotropic\", \"oriented\""},
        {"-srr-epsilon", 7, &optionTypes.floatType, &elementHierarchyState.epsilon, DEFAULT_ACTION,
         "-srr-epsilon <float>        : link power threshold (relative w.r.t. max. selfemitted power)"},
        {"-srr-minarea", 7, &optionTypes.floatType, &elementHierarchyState.minimumArea, DEFAULT_ACTION,
         "-srr-minarea <float>        : minimal element area (relative w.r.t. total area)"},
        {"-srr-display", 7, &showWhatTypeStruct, &stochasticRelaxationState.show, DEFAULT_ACTION,
         "-srr-display <what>         : \"total-radiance\", \"indirect-radiance\", \"weighting-gain\", \"importance\""},
        {"-srr-discard-incremental", 7, &optionTypes.boolType, &stochasticRelaxationState.discardIncremental, DEFAULT_ACTION,
         "-srr-discard-incremenal <y|n>: Discard result of first iteration (incremental steps)"},
        {"-srr-incremental-uses-importance", 7, &optionTypes.boolType, &stochasticRelaxationState.incrementalUsesImportance, DEFAULT_ACTION,
         "-srr-incremental-uses-importance <y|n>: Use view-importance sampling already for the first iteration (incremental steps)"},
        {"-srr-naive-merging", 7, &optionTypes.boolType, &stochasticRelaxationState.naiveMerging, DEFAULT_ACTION,
         "-srr-naive-merging <y|n>    : disable intelligent merging heuristic"},
        {"-srr-nondiffuse-first-shot", 7, &optionTypes.boolType, &stochasticRelaxationState.doNonDiffuseFirstShot, DEFAULT_ACTION,
         "-srr-nondiffuse-first-shot <y|n>: Do Non-diffuse first shot before real work"},
        {"-srr-initial-ls-samples", 7, &optionTypes.intType, &stochasticRelaxationState.initialLightSourceSamples, DEFAULT_ACTION,
         "-srr-initial-ls-samples <int>        : nr of samples per light source for initial shot"},
        {nullptr, 0, nullptr, nullptr, DEFAULT_ACTION, nullptr}
    };

    Options::parseGeneralOptions(srrOptions, argc, argv);
}

void
CommandLine::randomWalkRadiosityParseOptions(
        int *argc,
        char **argv,
        StochasticRelaxation &stochasticRelaxationState,
        OptionsType &optionTypes)
{
    CommandLineOptionDescription rwrOptions[] = {
        {"-rwr-ray-units", 8, &optionTypes.intType, &stochasticRelaxationState.rayUnitsPerIt, DEFAULT_ACTION,
         "-rwr-ray-units <n>          : To tune the amount of work in a single iteration"},
        {"-rwr-continuous", 7, &optionTypes.boolType, &stochasticRelaxationState.continuousRandomWalk, DEFAULT_ACTION,
         "-rwr-continuous <y|n>       : Continuous (yes) or Discrete (no) random walk"},
        {"-rwr-control-variate", 7, &optionTypes.boolType, &stochasticRelaxationState.constantControlVariate, DEFAULT_ACTION,
         "-rwr-control-variate <y|n>  : Constant Control Variate variance reduction"},
        {"-rwr-indirect-only", 7, &optionTypes.boolType, &stochasticRelaxationState.indirectOnly, DEFAULT_ACTION,
         "-rwr-indirect-only <y|n>    : Compute indirect illumination only"},
        {"-rwr-sampling-sequence", 7, &estTypeTypeStruct, &stochasticRelaxationState.sequence, DEFAULT_ACTION,
         "-rwr-sampling-sequence <type>: \"PseudoRandom\", \"Halton\", \"Niederreiter\""},
        {"-rwr-approximation", 7, &approxTypeStruct, &stochasticRelaxationState.approximationOrderType, DEFAULT_ACTION,
         "-rwr-approximation <order>  : \"constant\", \"linear\", \"quadratic\", \"cubic\""},
        {"-rwr-estimator", 7, &estTypeTypeStruct, &stochasticRelaxationState.randomWalkEstimatorType, DEFAULT_ACTION,
         "-rwr-estimator <type>       : \"shooting\", \"gathering\""},
        {"-rwr-score", 7, &estKindTypeStruct, &stochasticRelaxationState.randomWalkEstimatorKind, DEFAULT_ACTION,
         "-rwr-score <kind>           : \"collision\", \"absorption\", \"survival\", \"last-N\", \"last-but-N\""},
        {"-rwr-numlast", 12, &optionTypes.intType, &stochasticRelaxationState.randomWalkNumLast, DEFAULT_ACTION,
         "-rwr-numlast <int>          : N to use in \"last-N\" and \"last-but-N\" scorers"},
        {nullptr, 0, nullptr, nullptr, DEFAULT_ACTION, nullptr}
    };

    Options::parseGeneralOptions(rwrOptions, argc, argv);
}

static EnumDesc globalRayMatterPixelFilters[] = {
    {RayMatterFilterType::BOX_FILTER, "box", 2},
    {RayMatterFilterType::TENT_FILTER, "tent", 2},
    {RayMatterFilterType::GAUSS_FILTER, "gaussian 1/sqrt2", 2},
    {RayMatterFilterType::GAUSS2_FILTER, "gaussian 1/2", 2},
    {0, nullptr, 0}
};
static CommandLineOptions rmPixelFilterTypeStruct = Options::makeEnumOptTypeStruct(globalRayMatterPixelFilters);

void
CommandLine::rayMattingParseOptions(
        int *argc,
        char **argv,
        RayMatterState &rayMatterState,
        OptionsType &optionTypes)
{
    CommandLineOptionDescription rayMatterOptions[] =
    {
        {"-rm-samples-per-pixel", 6, &optionTypes.intType, &rayMatterState.samplesPerPixel, DEFAULT_ACTION,
         "-rm-samples-per-pixel <number>\t: eye-rays per pixel"},
        {"-rm-pixel-filter", 7, &rmPixelFilterTypeStruct, &rayMatterState.filter, DEFAULT_ACTION,
         "-rm-pixel-filter <type>\t: Select filter - \"box\", \"tent\", \"gaussian 1/sqrt2\", \"gaussian 1/2\""},
        {nullptr, 0, nullptr, nullptr, DEFAULT_ACTION, nullptr}
    };

    Options::parseGeneralOptions(rayMatterOptions, argc, argv);
}

/*** Enum Option types ***/

static EnumDesc globalRadModeValues[] = {
    {RayTracingRadMode::STORED_NONE, "none", 2},
    {RayTracingRadMode::STORED_DIRECT, "direct", 2},
    {RayTracingRadMode::STORED_INDIRECT, "indirect", 2},
    {RayTracingRadMode::STORED_PHOTON_MAP, "photonmap", 2},
    {0, nullptr, 0}
};

static CommandLineOptions radModeTypeStruct = Options::makeEnumOptTypeStruct(globalRadModeValues);

static EnumDesc globalLightModeValues[] = {
    {RayTracingLightMode::POWER_LIGHTS, "power", 2},
    {RayTracingLightMode::IMPORTANT_LIGHTS, "important", 2},
    {RayTracingLightMode::ALL_LIGHTS, "all", 2},
    {0, nullptr, 0}
};

static CommandLineOptions lightModeTypeStruct = Options::makeEnumOptTypeStruct(globalLightModeValues);

static EnumDesc globalSamplingModeValues[] = {
    {RayTracingSamplingMode::BRDF_SAMPLING, "bsdf", 2},
    {RayTracingSamplingMode::CLASSICAL_SAMPLING, "classical", 2},
    {0, nullptr, 0}
};
static CommandLineOptions samplingModeTypeStruct = Options::makeEnumOptTypeStruct(globalSamplingModeValues);

void
CommandLine::stochasticRayTracerParseOptions(
        int *argc,
        char **argv,
        StochasticRayTracingState &stochasticRayTracingState,
        OptionsType &optionTypes)
{
    CommandLineOptionDescription stochasticRatTracerOptions[] = {
        {"-rts-samples-per-pixel", 7, &optionTypes.intType, &stochasticRayTracingState.samplesPerPixel, DEFAULT_ACTION,
         "-rts-samples-per-pixel <number>\t: eye-rays per pixel"},
        {"-rts-no-progressive", 9, &optionTypes.setFalseType, &stochasticRayTracingState.progressiveTracing, DEFAULT_ACTION,
         "-rts-no-progressive\t: don't do progressive image refinement"},
        {"-rts-rad-mode", 8, &radModeTypeStruct, &stochasticRayTracingState.radMode, DEFAULT_ACTION,
         "-rts-rad-mode <type>\t: Stored radiance usage - \"none\", \"direct\", \"indirect\", \"photonmap\""},
        {"-rts-no-lightsampling", 9, &optionTypes.setFalseType, &stochasticRayTracingState.nextEvent, DEFAULT_ACTION,
         "-rts-no-lightsampling\t: don't do explicit light sampling"},
        {"-rts-l-mode", 8, &lightModeTypeStruct, &stochasticRayTracingState.lightMode, DEFAULT_ACTION,
         "-rts-l-mode <type>\t: Light sampling mode - \"power\", \"important\", \"all\""},
        {"-rts-l-samples", 8, &optionTypes.intType, &stochasticRayTracingState.nextEventSamples, DEFAULT_ACTION,
         "-rts-l-samples <number>\t: explicit light source samples at each hit"},
        {"-rts-scatter-samples", 7, &optionTypes.intType, &stochasticRayTracingState.scatterSamples, DEFAULT_ACTION,
         "-rts-scatter-samples <number>\t: scattered rays at each bounce"},
        {"-rts-do-fdg", 0, &optionTypes.setTrueType, &stochasticRayTracingState.differentFirstDG, DEFAULT_ACTION,
         "-rts-do-fdg\t: use different nr. of scatter samples for first diffuse/glossy bounce"},
        {"-rts-fdg-samples", 8, &optionTypes.intType, &stochasticRayTracingState.firstDGSamples, DEFAULT_ACTION,
         "-rts-fdg-samples <number>\t: scattered rays at first diffuse/glossy bounce"},
        {"-rts-separate-specular", 8, &optionTypes.setTrueType, &stochasticRayTracingState.separateSpecular, DEFAULT_ACTION,
         "-rts-separate-specular\t: always shoot separate rays for specular scattering"},
        {"-rts-s-mode", 9, &samplingModeTypeStruct, &stochasticRayTracingState.reflectionSampling, DEFAULT_ACTION,
         "-rts-s-mode <type>\t: Sampling mode - \"bsdf\", \"classical\""},
        {"-rts-min-path-length", 8, &optionTypes.intType, &stochasticRayTracingState.minPathDepth, DEFAULT_ACTION,
         "-rts-min-path-length <number>\t: minimum path length before Russian roulette"},
        {"-rts-max-path-length", 8, &optionTypes.intType, &stochasticRayTracingState.maxPathDepth, DEFAULT_ACTION,
         "-rts-max-path-length <number>\t: maximum path length (ignoring higher orders)"},
        {"-rts-NOdirect-background-rad", 8, &optionTypes.setFalseType, &stochasticRayTracingState.backgroundDirect, DEFAULT_ACTION,
         "-rts-NOdirect-background-rad\t: patchIsOnOmitSet direct background radiance."},
        {"-rts-NOindirect-background-rad", 8, &optionTypes.setFalseType, &stochasticRayTracingState.backgroundIndirect, DEFAULT_ACTION,
         "-rts-NOindirect-background-rad\t: patchIsOnOmitSet indirect background radiance."},
        {nullptr, 0, nullptr, nullptr, DEFAULT_ACTION, nullptr}
    };

    Options::parseGeneralOptions(stochasticRatTracerOptions, argc, argv);
}

static CommandLineOptions RegExpStringType = Options::makeNStringTypeStruct(MAX_REGEXP_SIZE);

void
CommandLine::biDirectionalPathParseOptions(
        int *argc,
        char **argv,
        BidirectionalPathTracingState &bidirectionalPathState,
        OptionsType &optionTypes)
{
    CommandLineOptionDescription bidirectionalOptions[] = {
        {"-bidir-samples-per-pixel", 8, &optionTypes.intType, &bidirectionalPathState.baseConfig.samplesPerPixel, DEFAULT_ACTION,
        "-bidir-samples-per-pixel <number> : eye-rays per pixel"},
        {"-bidir-no-progressive", 11, &optionTypes.setFalseType, &bidirectionalPathState.baseConfig.progressiveTracing, DEFAULT_ACTION,
        "-bidir-no-progressive          \t: don't do progressive image refinement"},
        {"-bidir-max-eye-path-length", 12, &optionTypes.intType, &bidirectionalPathState.baseConfig.maximumEyePathDepth, DEFAULT_ACTION,
        "-bidir-max-eye-path-length <number>: maximum eye path length"},
        {"-bidir-max-light-path-length", 12, &optionTypes.intType, &bidirectionalPathState.baseConfig.maximumLightPathDepth, DEFAULT_ACTION,
        "-bidir-max-light-path-length <number>: maximum light path length"},
        {"-bidir-max-path-length", 12, &optionTypes.intType, &bidirectionalPathState.baseConfig.maximumPathDepth, DEFAULT_ACTION,
        "-bidir-max-path-length <number>\t: maximum combined path length"},
        {"-bidir-min-path-length", 12, &optionTypes.intType, &bidirectionalPathState.baseConfig.minimumPathDepth, DEFAULT_ACTION,
        "-bidir-min-path-length <number>\t: minimum path length before russian roulette"},
        {"-bidir-no-light-importance", 11, &optionTypes.setFalseType, &bidirectionalPathState.baseConfig.sampleImportantLights, DEFAULT_ACTION,
        "-bidir-no-light-importance     \t: sample lights based on power, ignoring their importance"},
        {"-bidir-use-regexp", 12, &optionTypes.setTrueType, &bidirectionalPathState.baseConfig.useSpars, DEFAULT_ACTION,
        "-bidir-use-regexp\t: use regular expressions for path evaluation"},
        {"-bidir-use-emitted", 12, &optionTypes.boolType, &bidirectionalPathState.baseConfig.doLe, DEFAULT_ACTION,
        "-bidir-use-emitted <yes|no>\t: use reg exp for emitted radiance"},
        {"-bidir-rexp-emitted", 13, &RegExpStringType, bidirectionalPathState.baseConfig.leRegExp, DEFAULT_ACTION,
        "-bidir-rexp-emitted <string>\t: reg exp for emitted radiance"},
        {"-bidir-reg-direct", 12, &optionTypes.boolType, &bidirectionalPathState.baseConfig.doLD, DEFAULT_ACTION,
        "-bidir-reg-direct <yes|no>\t: use reg exp for stored direct illumination (galerkin!)"},
        {"-bidir-rexp-direct", 13, &RegExpStringType, bidirectionalPathState.baseConfig.ldRegExp, DEFAULT_ACTION,
        "-bidir-rexp-direct <string>\t: reg exp for stored direct illumination"},
        {"-bidir-reg-indirect", 12, &optionTypes.boolType, &bidirectionalPathState.baseConfig.doLI, DEFAULT_ACTION,
        "-bidir-reg-indirect <yes|no>\t: use reg exp for stored indirect illumination (galerkin!)"},
        {"-bidir-rexp-indirect", 13, &RegExpStringType, bidirectionalPathState.baseConfig.liRegExp, DEFAULT_ACTION,
        "-bidir-rexp-indirect <string>\t: reg exp for stored indirect illumination"},
        {nullptr, 0, nullptr, nullptr, DEFAULT_ACTION, nullptr}
    };

    Options::parseGeneralOptions(bidirectionalOptions, argc, argv);
}

static char *globalRaytracingMethodsString;
static char *globalRayTracerName;

void
CommandLine::mainRayTracingOption(void *value) {
    const char *name = *static_cast<char **>(value);
    strcpy(globalRayTracerName, name);
}

void
CommandLine::rayTracingParseOptions(
        int *argc,
        char **argv,
        char raytracingMethodsString[],
        char *rayTracerName,
        OptionsType &optionTypes)
{
    CommandLineOptionDescription globalRaytracingOptions[] = {
        {"-raytracing-method", 4, &optionTypes.stringType,  nullptr, CommandLine::mainRayTracingOption, globalRaytracingMethodsString},
        {nullptr, 0, nullptr, nullptr, DEFAULT_ACTION, nullptr}
    };

    globalRayTracerName = rayTracerName;
    globalRaytracingMethodsString = raytracingMethodsString;
    Options::parseGeneralOptions(globalRaytracingOptions, argc, argv);
}

void
CommandLine::photonMapParseOptions(
        int *argc,
        char **argv,
        PhotonMapState &photonMapState,
        OptionsType &optionTypes)
{
    CommandLineOptionDescription photonMapOptions[] = {
        {"-pmap-do-global", 9, &optionTypes.boolType, &photonMapState.doGlobalMap, DEFAULT_ACTION,
         "-pmap-do-global <true|false> : Trace photons for the global map"},
        {"-pmap-global-paths", 9, &optionTypes.intType, &photonMapState.gPathsPerIteration, DEFAULT_ACTION,
         "-pmap-global-paths <number> : Number of paths per iteration for the global map"},
        {"-pmap-g-preirradiance", 11, &optionTypes.boolType, &photonMapState.precomputeGIrradiance, DEFAULT_ACTION,
         "-pmap-g-preirradiance <true|false> : Use irradiance precomputation for global map"},
        {"-pmap-do-caustic", 9, &optionTypes.boolType, &photonMapState.doCausticMap, DEFAULT_ACTION,
         "-pmap-do-caustic <true|false> : Trace photons for the caustic map"},
        {"-pmap-caustic-paths", 9,  &optionTypes.intType, &photonMapState.cPathsPerIteration, DEFAULT_ACTION,
         "-pmap-caustic-paths <number> : Number of paths per iteration for the caustic map"},
        {"-pmap-render-hits", 9, &optionTypes.setTrueType, &photonMapState.renderImage, DEFAULT_ACTION,
         "-pmap-render-hits: Show photon hits on screen"},
        {"-pmap-recon-gphotons", 9, &optionTypes.intType, &photonMapState.reconGPhotons, DEFAULT_ACTION,
         "-pmap-recon-cphotons <number> : Number of photons to use in reconstructions (global map)"},
        {"-pmap-recon-iphotons", 9, &optionTypes.intType, &photonMapState.reconCPhotons, DEFAULT_ACTION,
         "-pmap-recon-photons <number> : Number of photons to use in reconstructions (caustic map)"},
        {"-pmap-recon-photons", 9, &optionTypes.intType, &photonMapState.reconIPhotons, DEFAULT_ACTION,
         "-pmap-recon-photons <number> : Number of photons to use in reconstructions (importance)"},
        {"-pmap-balancing", 9, &optionTypes.boolType, &photonMapState.balanceKDTree, DEFAULT_ACTION,
         "-pmap-balancing <true|false> : Balance KD Tree before raytracing"},
        {nullptr, 0, nullptr, nullptr, DEFAULT_ACTION, nullptr}
    };
    Options::parseGeneralOptions(photonMapOptions, argc, argv);
}

#endif
