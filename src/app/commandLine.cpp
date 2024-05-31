#include <cstring>
#include "common/error.h"
#include "common/RenderOptions.h"
#include "scene/Camera.h"
#include "tonemap/ToneMap.h"
#include "GALERKIN/GalerkinRadianceMethod.h"

#ifdef RAYTRACING_ENABLED
    #include "raycasting/simple/RayMatter.h"
    #include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
    #include "raycasting/stochasticRaytracing/basismcrad.h"
    #include "raycasting/stochasticRaytracing/HierarchyClusteringMode.h"
    #include "raycasting/stochasticRaytracing/sample4d.h"
    #include "raycasting/stochasticRaytracing/mcradP.h"
    #include "raycasting/stochasticRaytracing/hierarchy.h"
    #include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
    #include "PHOTONMAP/pmapoptions.h"
#endif

#include "app/options.h"
#include "app/BatchOptions.h"
#include "app/commandLine.h"

// Default scene level configuration
static const int DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS = 4;
static const bool DEFAULT_FORCE_ONE_SIDED = true;

// Default virtual camera
static const Vector3D DEFAULT_CAMERA_EYE_POSITION(10.0, 0.0, 0.0);
static const Vector3D DEFAULT_CAMERA_LOOK_POSITION(0.0, 0.0, 0.0);
static const Vector3D DEFAULT_CAMERA_UP_DIRECTION(0.0, 0.0, 1.0);
static const ColorRgb DEFAULT_BACKGROUND_COLOR(0.0, 0.0, 0.0);
static const float DEFAULT_CAMERA_FIELD_OF_VIEW = 22.5f;
static int globalNumberOfQuarterCircleDivisions = DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
static int globalFileOptionsForceOneSidedSurfaces = 0;
static int globalYes = 1;
static int globalNo = 0;
static int globalOutputImageWidth = 1920;
static int globalOutputImageHeight = 1080;
static Camera globalCamera;

static void
mainForceOneSidedOption(void *value) {
    globalFileOptionsForceOneSidedSurfaces = *((int *) value);
}

static void
mainMonochromeOption(void *value) {
    globalNumberOfQuarterCircleDivisions = *(int *)value;
}

static void
commandLineImageWidthOption(void *value) {
    globalOutputImageWidth = *(int *)value;
}

static void
commandLineImageHeightOption(void *value) {
    globalOutputImageHeight = *(int *)value;
}

static CommandLineOptionDescription globalOptions[] = {
    {"-nqcdivs", 3, &GLOBAL_options_intType, &globalNumberOfQuarterCircleDivisions, DEFAULT_ACTION,
     "-nqcdivs <integer>\t: number of quarter circle divisions"},
    {"-force-onesided", 10, TYPELESS, &globalYes, mainForceOneSidedOption,
     "-force-onesided\t\t: force one-sided surfaces"},
    {"-dont-force-onesided", 14, TYPELESS, &globalNo, mainForceOneSidedOption,
     "-dont-force-onesided\t: allow two-sided surfaces"},
    {"-monochromatic", 5, TYPELESS, &globalYes, mainMonochromeOption,
     "-monochromatic \t\t: convert colors to shades of grey"},
    {"-width", 5, &GLOBAL_options_intType, &globalOutputImageWidth, commandLineImageWidthOption,
            "-width \t\t: image output width in pixels"},
    {"-height", 6, &GLOBAL_options_intType, &globalOutputImageHeight, commandLineImageHeightOption,
            "-width \t\t: image output width in pixels"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void
commandLineGeneralProgramParseOptions(
    int *argc,
    char **argv,
    bool *oneSidedSurfaces,
    int *conicSubDivisions,
    int *imageOutputWidth,
    int *imageOutputHeight)
{
    globalFileOptionsForceOneSidedSurfaces = DEFAULT_FORCE_ONE_SIDED;
    globalNumberOfQuarterCircleDivisions = DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
    parseGeneralOptions(globalOptions, argc, argv); // Order is important, this should be called last

    if ( globalFileOptionsForceOneSidedSurfaces != 0 ) {
        *oneSidedSurfaces = true;
    } else {
        *oneSidedSurfaces = false;
    }
    *conicSubDivisions = globalNumberOfQuarterCircleDivisions;
    *imageOutputWidth = globalOutputImageWidth;
    *imageOutputHeight = globalOutputImageHeight;
}

static void
cameraSetEyePositionOption(void *val) {
    const Vector3D *v = (Vector3D *)val;
    globalCamera.setEyePosition(v->x, v->y, v->z);
}

static void
cameraSetLookPositionOption(void *val) {
    const Vector3D *v = (Vector3D *)val;
    globalCamera.setLookPosition(v->x, v->y, v->z);
}

static void
cameraSetUpDirectionOption(void *val) {
    const Vector3D *v = (Vector3D *)val;
    globalCamera.setUpDirection(v->x, v->y, v->z);
}

static void cameraSetFieldOfViewOption(void *val) {
    const float *v = (float *)val;
    globalCamera.setFieldOfView(*v);
}

static CommandLineOptionDescription globalCameraOptions[] = {
    {"-eyepoint", 4, TVECTOR, &globalCamera.eyePosition, cameraSetEyePositionOption,
     "-eyepoint  <vector>\t: viewing position"},
    {"-center", 4, TVECTOR, &globalCamera.lookPosition, cameraSetLookPositionOption,
     "-center    <vector>\t: point looked at"},
    {"-updir", 3, TVECTOR, &globalCamera.upDirection, cameraSetUpDirectionOption,
     "-updir     <vector>\t: direction pointing up"},
    {"-fov", 4, Tfloat,  &globalCamera.fieldOfVision, cameraSetFieldOfViewOption,
     "-fov       <float> \t: field of view angle"},
    {nullptr, 0, TYPELESS, nullptr, nullptr, nullptr}
};

static void
cameraDefaults(Camera *camera, int imageWidth, int imageHeight) {
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
cameraParseOptions(int *argc, char **argv, Camera *camera, int imageWidth, int imageHeight) {
    cameraDefaults(&globalCamera, imageWidth, imageHeight);
    parseGeneralOptions(globalCameraOptions, argc, argv);
    *camera = globalCamera;
}

// Used for option management
static int globalTrue = true;
static int globalFalse = false;

static void
iterationMethodOption(void *value) {
    char *name = *(char **)value;

    if ( strncasecmp(name, "jacobi", 2) == 0 ) {
        GalerkinRadianceMethod::galerkinState.galerkinIterationMethod = JACOBI;
    } else if ( strncasecmp(name, "gaussseidel", 2) == 0 ) {
        GalerkinRadianceMethod::galerkinState.galerkinIterationMethod = GAUSS_SEIDEL;
    } else if ( strncasecmp(name, "southwell", 2) == 0 ) {
        GalerkinRadianceMethod::galerkinState.galerkinIterationMethod = SOUTH_WELL;
    } else {
        logError(nullptr, "Invalid iteration method '%s'", name);
    }
}

static void
hierarchicalOption(void *value) {
    int yesno = *(int *)value;

    if ( yesno != 0 ) {
        GalerkinRadianceMethod::galerkinState.hierarchical = true;
    } else {
        GalerkinRadianceMethod::galerkinState.hierarchical = false;
    }
}

static void
lazyOption(void *value) {
    int yesno = *(int *) value;
    GalerkinRadianceMethod::galerkinState.lazyLinking = yesno;
}

static void
clusteringOption(void *value) {
    int yesno = *(int *) value;
    GalerkinRadianceMethod::galerkinState.clustered = yesno;
}

static void
importanceOption(void *value) {
    int yesno = *(int *) value;
    GalerkinRadianceMethod::galerkinState.importanceDriven = yesno;
}

static void
ambientOption(void *value) {
    int yesno = *(int *) value;
    GalerkinRadianceMethod::galerkinState.useAmbientRadiance = yesno;
}

static CommandLineOptionDescription galerkinOptions[] = {
        {"-gr-iteration-method", 6, Tstring, nullptr, iterationMethodOption,
                                                "-gr-iteration-method <methodname>: Jacobi, GaussSeidel, Southwell"},
        {"-gr-hierarchical", 6, TYPELESS, (void *)&globalTrue, hierarchicalOption,
                                                "-gr-hierarchical    \t: do hierarchical refinement"},
        {"-gr-not-hierarchical", 10, TYPELESS, (void *)&globalFalse, hierarchicalOption,
                                                "-gr-not-hierarchical\t: don't do hierarchical refinement"},
        {"-gr-lazy-linking", 6, TYPELESS, (void *)&globalTrue, lazyOption,
                                                "-gr-lazy-linking    \t: do lazy linking"},
        {"-gr-no-lazy-linking", 10, TYPELESS, (void *)&globalFalse, lazyOption,
                                                "-gr-no-lazy-linking \t: don't do lazy linking"},
        {"-gr-clustering", 6, TYPELESS, (void *)&globalTrue, clusteringOption,
                                                "-gr-clustering      \t: do clustering"},
        {"-gr-no-clustering", 10, TYPELESS, (void *)&globalFalse, clusteringOption,
                                                "-gr-no-clustering   \t: don't do clustering"},
        {"-gr-importance", 6, TYPELESS, (void *)&globalTrue, importanceOption,
                                                "-gr-importance      \t: do view-potential driven computations"},
        {"-gr-no-importance", 10, TYPELESS, (void *)&globalFalse, importanceOption,
                                                "-gr-no-importance   \t: don't use view-potential"},
        {"-gr-ambient", 6, TYPELESS, (void *)&globalTrue, ambientOption,
                                                "-gr-ambient         \t: do visualisation with ambient term"},
        {"-gr-no-ambient", 10, TYPELESS, (void *)&globalFalse, ambientOption,
                                                "-gr-no-ambient      \t: do visualisation without ambient term"},
        {"-gr-link-error-threshold", 6, Tfloat, &GalerkinRadianceMethod::galerkinState.relLinkErrorThreshold, nullptr,
                                                "-gr-link-error-threshold <float>: Relative link error threshold"},
        {"-gr-min-elem-area", 6, Tfloat, &GalerkinRadianceMethod::galerkinState.relMinElemArea, nullptr,
                                                "-gr-min-elem-area <float> \t: Relative element area threshold"},
        {nullptr, 0, nullptr, nullptr, nullptr, nullptr}
};

void
galerkinParseOptions(int *argc, char **argv) {
    parseGeneralOptions(galerkinOptions, argc, argv);
}

// Composes explanation for -tonemapping command line option
#define STRING_LENGTH 1000
static char globalToneMappingMethodsString[STRING_LENGTH];
static float globalRxy[2];
static float globalGxy[2];
static float globalBxy[2];
static float globalWxy[2];

static void
makeToneMappingMethodsString() {
    strcpy(globalToneMappingMethodsString,
       "-tonemapping <method>: Set tone mapping method\n"
       "\tmethods: Lightness            Lightness Mapping (default)\n"
       "\t         TumblinRushmeier     Tumblin/Rushmeier's Mapping\n"
       "\t         Ward                 Ward's Mapping\n"
       "\t         RevisedTR            Revised Tumblin/Rushmeier's Mapping\n"
       "\t         Ferwerda             Partial Ferwerda's Mapping");
}

static char *globalToneMapName;

static void
toneMappingMethodOption(void *value) {
    char *name = *(char **)value;

    strcpy(globalToneMapName, name);
}

static void
brightnessAdjustOption(void * /*val*/) {
    GLOBAL_toneMap_options.pow_bright_adjust = java::Math::pow(2.0f, GLOBAL_toneMap_options.brightness_adjust);
}

static void
chromaOption(void *value) {
    const float *chroma = (float *)value;
    if ( chroma == globalRxy ) {
        GLOBAL_toneMap_options.xr = chroma[0];
        GLOBAL_toneMap_options.yr = chroma[1];
    } else if ( chroma == globalGxy ) {
        GLOBAL_toneMap_options.xg = chroma[0];
        GLOBAL_toneMap_options.yg = chroma[1];
    } else if ( chroma == globalBxy ) {
        GLOBAL_toneMap_options.xb = chroma[0];
        GLOBAL_toneMap_options.yb = chroma[1];
    } else if ( chroma == globalWxy ) {
        GLOBAL_toneMap_options.xw = chroma[0];
        GLOBAL_toneMap_options.yw = chroma[1];
    } else {
        logFatal(-1, "chromaOption", "invalid value pointer");
    }

    computeColorConversionTransforms(
        GLOBAL_toneMap_options.xr, GLOBAL_toneMap_options.yr,
        GLOBAL_toneMap_options.xg, GLOBAL_toneMap_options.yg,
        GLOBAL_toneMap_options.xb, GLOBAL_toneMap_options.yb,
        GLOBAL_toneMap_options.xw, GLOBAL_toneMap_options.yw);
}

static void
toneMappingCommandLineOptionDescAdaptMethodOption(void *value) {
    char *name = *(char **) value;

    if ( strncasecmp(name, "average", 2) == 0 ) {
        GLOBAL_toneMap_options.staticAdaptationMethod = ToneMapAdaptationMethod::TMA_AVERAGE;
    } else if ( strncasecmp(name, "median", 2) == 0 ) {
        GLOBAL_toneMap_options.staticAdaptationMethod = ToneMapAdaptationMethod::TMA_MEDIAN;
    } else {
        logError(nullptr, "Invalid adaptation estimate method '%s'", name);
    }
}

static void
gammaOption(void *value) {
    float gam = *(float *) value;
    GLOBAL_toneMap_options.gamma.set(gam, gam, gam);
}

static CommandLineOptionDescription globalToneMappingOptions[] = {
    {"-tonemapping", 4, Tstring, nullptr, toneMappingMethodOption, globalToneMappingMethodsString},
    {"-brightness-adjust", 4, Tfloat, &GLOBAL_toneMap_options.brightness_adjust, brightnessAdjustOption,
     "-brightness-adjust <float> : brightness adjustment factor"},
    {"-adapt", 5, Tstring, nullptr, toneMappingCommandLineOptionDescAdaptMethodOption,
    "-adapt <method>  \t: adaptation estimation method\n\tmethods: \"average\", \"median\""},
    {"-lwa", 3, Tfloat, &GLOBAL_toneMap_options.realWorldAdaptionLuminance, DEFAULT_ACTION,
     "-lwa <float>\t\t: real world adaptation luminance"},
    {"-ldmax", 5, Tfloat, &GLOBAL_toneMap_options.maximumDisplayLuminance, DEFAULT_ACTION,
     "-ldmax <float>\t\t: maximum diaply luminance"},
    {"-cmax", 4, Tfloat, &GLOBAL_toneMap_options.maximumDisplayContrast, DEFAULT_ACTION,
     "-cmax <float>\t\t: maximum displayable contrast"},
    {"-gamma", 4, Tfloat, nullptr, gammaOption,
     "-gamma <float>       \t: gamma correction factor (same for red, green. blue)"},
    {"-rgbgamma", 4, TRGB, &GLOBAL_toneMap_options.gamma, DEFAULT_ACTION,
     "-rgbgamma <r> <g> <b>\t: gamma correction factor (separate for red, green, blue)"},
    {"-red", 4, Txy, globalRxy, chromaOption,
     "-red <xy>            \t: CIE xy chromaticity of monitor red"},
    {"-green", 4, Txy, globalGxy, chromaOption,
     "-green <xy>          \t: CIE xy chromaticity of monitor green"},
    {"-blue", 4, Txy, globalBxy, chromaOption,
     "-blue <xy>           \t: CIE xy chromaticity of monitor blue"},
    {"-white", 4, Txy, globalWxy, chromaOption,
     "-white <xy>          \t: CIE xy chromaticity of monitor white"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void
toneMapParseOptions(int *argc, char **argv, char *toneMapName) {
    globalToneMapName = toneMapName;
    makeToneMappingMethodsString();
    parseGeneralOptions(globalToneMappingOptions, argc, argv);
    recomputeGammaTables(GLOBAL_toneMap_options.gamma);
}

static char *globalRadianceMethodsString;

static CommandLineOptionDescription globalRadianceOptions[] = {
        {"-radiance-method", 4, Tstring,  nullptr, DEFAULT_ACTION, globalRadianceMethodsString},
        {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void
radianceMethodParseOptions(int *argc, char **argv, char *radianceMethodsString) {
    globalRadianceMethodsString = radianceMethodsString;
    parseGeneralOptions(globalRadianceOptions, argc, argv);
}

static RenderOptions globalRenderOptions;
static ColorRgb globalOutlineColor;

static void
flatOption(void * /*value*/) {
    globalRenderOptions.smoothShading = false;
}

static void
noCullingOption(void * /*value*/) {
    globalRenderOptions.backfaceCulling = false;
}

static void
outlinesOption(void * /*value*/) {
    globalRenderOptions.drawOutlines = true;
}

static void
traceOption(void * /*value*/) {
    globalRenderOptions.trace = true;
}

static CommandLineOptionDescription renderingOptions[] = {
    {"-flat-shading", 5, TYPELESS, nullptr, flatOption,
    "-flat-shading\t\t: render without Gouraud (color) interpolation"},
    {"-raycast", 5, TYPELESS, nullptr, traceOption,
    "-raycast\t\t: save raycasted scene view as a high dynamic range image"},
    {"-no-culling", 5, TYPELESS, nullptr, noCullingOption,
    "-no-culling\t\t: don't use backface culling"},
    {"-outlines", 5, TYPELESS, nullptr, outlinesOption,
    "-outlines\t\t: draw polygon outlines"},
    {"-outline-color", 10, TRGB, &globalOutlineColor, DEFAULT_ACTION,
    "-outline-color <rgb> \t: color for polygon outlines"},
    {nullptr, 0, nullptr, nullptr, nullptr, nullptr}
};

void
renderParseOptions(int *argc, char **argv, RenderOptions *renderOptions) {
    globalRenderOptions = *renderOptions;

    parseGeneralOptions(renderingOptions, argc, argv);

    *renderOptions = globalRenderOptions;
    renderOptions->outlineColor.r = globalOutlineColor.r;
    renderOptions->outlineColor.g = globalOutlineColor.g;
    renderOptions->outlineColor.b = globalOutlineColor.b;
}

static BatchOptions globalBatchOptions;

static CommandLineOptionDescription globalCommandLineBatchOptions[] = {
    {"-iterations", 3, &GLOBAL_options_intType, &globalBatchOptions.iterations, DEFAULT_ACTION,
    "-iterations <integer>\t: world-space radiance iterations"},
    {"-radiance-image-savefile", 12, Tstring, &globalBatchOptions.radianceImageFileNameFormat, DEFAULT_ACTION,
     "-radiance-image-savefile <filename>\t: radiance PPM/LOGLUV savefile name,\n\tfirst '%%d' will be substituted by iteration number"},
    {"-radiance-model-savefile", 12, Tstring, &globalBatchOptions.radianceModelFileNameFormat, DEFAULT_ACTION,
     "-radiance-model-savefile <filename>\t: radiance VRML model savefile name,"
     "\n\tfirst '%%d' will be substituted by iteration number"},
    {"-save-modulo", 8, &GLOBAL_options_intType, &globalBatchOptions.saveModulo, DEFAULT_ACTION,
     "-save-modulo <integer>\t: save every n-th iteration"},
    {"-raytracing-image-savefile", 14, Tstring, &globalBatchOptions.raytracingImageFileName, DEFAULT_ACTION,
     "-raytracing-image-savefile <filename>\t: raytracing PPM savefile name"},
    {"-timings", 3, Tsettrue, &globalBatchOptions.timings, DEFAULT_ACTION,
     "-timings\t: printRegularHierarchy timings for world-space radiance and raytracing methods"},
    {nullptr, 0,  TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void
batchParseOptions(int *argc, char **argv, BatchOptions *options) {
    globalBatchOptions = *options;
    parseGeneralOptions(globalCommandLineBatchOptions, argc, argv);
    *options = globalBatchOptions;
}

#ifdef RAYTRACING_ENABLED

static ENUMDESC globalApproximateValues[] = {
    {StochasticRaytracingApproximation::CONSTANT, "constant", 2},
    {StochasticRaytracingApproximation::LINEAR, "linear", 2},
    {StochasticRaytracingApproximation::BI_LINEAR, "bilinear", 2},
    {StochasticRaytracingApproximation::QUADRATIC, "quadratic", 2},
    {StochasticRaytracingApproximation::CUBIC, "cubic", 2},
    {0, nullptr, 0}
};
MakeEnumOptTypeStruct(approxTypeStruct, globalApproximateValues);

static ENUMDESC clusteringVals[] = {
    {HierarchyClusteringMode::NO_CLUSTERING, "none", 2},
    {HierarchyClusteringMode::ISOTROPIC_CLUSTERING, "isotropic", 2},
    {HierarchyClusteringMode::ORIENTED_CLUSTERING, "oriented",  2},
    {0, nullptr, 0}
};
MakeEnumOptTypeStruct(clusteringTypeStruct, clusteringVals);

static ENUMDESC sequenceVals[] = {
    {Sampler4DSequence::RANDOM, "PseudoRandom", 2},
    {Sampler4DSequence::HALTON,"Halton", 2},
    {Sampler4DSequence::NIEDERREITER, "Niederreiter", 2}, // TODO: Not able to select all available sequences...
    {0, nullptr, 0}
};
MakeEnumOptTypeStruct(sequenceTypeStruct, sequenceVals);

static ENUMDESC estTypeVals[] = {
    {RW_SHOOTING, "Shooting", 2},
    {RW_GATHERING, "Gathering", 2},
    {0, nullptr, 0}
};
MakeEnumOptTypeStruct(estTypeTypeStruct, estTypeVals);

static ENUMDESC globalEstKindValues[] = {
    {RW_COLLISION, "Collision", 2},
    {RW_ABSORPTION, "Absorption", 2},
    {RW_SURVIVAL, "Survival", 2},
    {RW_LAST_BUT_NTH, "Last-but-N", 2},
    {RW_N_LAST, "Last-N", 2},
    {0, nullptr, 0}
};
MakeEnumOptTypeStruct(estKindTypeStruct, globalEstKindValues);

static ENUMDESC showWhatVals[] = {
    {SHOW_TOTAL_RADIANCE, "total-radiance", 2},
    {SHOW_INDIRECT_RADIANCE, "indirect-radiance", 2},
    {SHOW_IMPORTANCE, "importance", 2},
    {0, nullptr, 0}
};
MakeEnumOptTypeStruct(showWhatTypeStruct, showWhatVals);

static CommandLineOptionDescription srrOptions[] = {
    {"-srr-ray-units", 8, &GLOBAL_options_intType, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.rayUnitsPerIt, DEFAULT_ACTION,
     "-srr-ray-units <n>          : To tune the amount of work in a single iteration"},
    {"-srr-bidirectional", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.bidirectionalTransfers, DEFAULT_ACTION,
     "-srr-bidirectional <yes|no> : Use lines bidirectionally"},
    {"-srr-control-variate", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate, DEFAULT_ACTION,
     "-srr-control-variate <y|n>  : Constant Control Variate variance reduction"},
    {"-srr-indirect-only", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly, DEFAULT_ACTION,
     "-srr-indirect-only <y|n>    : Compute indirect illumination only"},
    {"-srr-importance-driven", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven, DEFAULT_ACTION,
     "-srr-importance-driven <y|n>: Use view-importance"},
    {"-srr-sampling-sequence", 7, &sequenceTypeStruct, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sequence, DEFAULT_ACTION,
     "-srr-sampling-sequence <type>: \"PseudoRandom\", \"Niederreiter\""},
    {"-srr-approximation", 7, &approxTypeStruct, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType, DEFAULT_ACTION,
     "-srr-approximation <order>  : \"constant\", \"linear\", \"quadratic\", \"cubic\""},
    {"-srr-hierarchical", 7, Tbool, &GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing, DEFAULT_ACTION,
     "-srr-hierarchical <y|n>     : hierarchical refinement"},
    {"-srr-clustering", 7, &clusteringTypeStruct, &GLOBAL_stochasticRaytracing_hierarchy.clustering, DEFAULT_ACTION,
     "-srr-clustering <mode>      : \"none\", \"isotropic\", \"oriented\""},
    {"-srr-epsilon", 7, Tfloat, &GLOBAL_stochasticRaytracing_hierarchy.epsilon, DEFAULT_ACTION,
     "-srr-epsilon <float>        : link power threshold (relative w.r.t. max. selfemitted power)"},
    {"-srr-minarea", 7, Tfloat, &GLOBAL_stochasticRaytracing_hierarchy.minimumArea, DEFAULT_ACTION,
     "-srr-minarea <float>        : minimal element area (relative w.r.t. total area)"},
    {"-srr-display", 7, &showWhatTypeStruct, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show, DEFAULT_ACTION,
     "-srr-display <what>         : \"total-radiance\", \"indirect-radiance\", \"weighting-gain\", \"importance\""},
    {"-srr-discard-incremental", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.discardIncremental, DEFAULT_ACTION,
     "-srr-discard-incremenal <y|n>: Discard result of first iteration (incremental steps)"},
    {"-srr-incremental-uses-importance", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.incrementalUsesImportance, DEFAULT_ACTION,
     "-srr-incremental-uses-importance <y|n>: Use view-importance sampling already for the first iteration (incremental steps)"},
    {"-srr-naive-merging", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.naiveMerging, DEFAULT_ACTION,
     "-srr-naive-merging <y|n>    : disable intelligent merging heuristic"},
    {"-srr-nondiffuse-first-shot", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.doNonDiffuseFirstShot, DEFAULT_ACTION,
     "-srr-nondiffuse-first-shot <y|n>: Do Non-diffuse first shot before real work"},
    {"-srr-initial-ls-samples", 7, &GLOBAL_options_intType, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialLightSourceSamples, DEFAULT_ACTION,
     "-srr-initial-ls-samples <int>        : nr of samples per light source for initial shot"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

static CommandLineOptionDescription rwrOptions[] = {
    {"-rwr-ray-units", 8, &GLOBAL_options_intType, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.rayUnitsPerIt, DEFAULT_ACTION,
     "-rwr-ray-units <n>          : To tune the amount of work in a single iteration"},
    {"-rwr-continuous", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk, DEFAULT_ACTION,
     "-rwr-continuous <y|n>       : Continuous (yes) or Discrete (no) random walk"},
    {"-rwr-control-variate", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate, DEFAULT_ACTION,
     "-rwr-control-variate <y|n>  : Constant Control Variate variance reduction"},
    {"-rwr-indirect-only", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly, DEFAULT_ACTION,
     "-rwr-indirect-only <y|n>    : Compute indirect illumination only"},
    {"-rwr-sampling-sequence", 7, &estTypeTypeStruct, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sequence, DEFAULT_ACTION,
     "-rwr-sampling-sequence <type>: \"PseudoRandom\", \"Halton\", \"Niederreiter\""},
    {"-rwr-approximation", 7, &approxTypeStruct, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType, DEFAULT_ACTION,
     "-rwr-approximation <order>  : \"constant\", \"linear\", \"quadratic\", \"cubic\""},
    {"-rwr-estimator", 7, &estTypeTypeStruct, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorType, DEFAULT_ACTION,
     "-rwr-estimator <type>       : \"shooting\", \"gathering\""},
    {"-rwr-score", 7, &estKindTypeStruct, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorKind, DEFAULT_ACTION,
     "-rwr-score <kind>           : \"collision\", \"absorption\", \"survival\", \"last-N\", \"last-but-N\""},
    {"-rwr-numlast", 12, &GLOBAL_options_intType, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkNumLast, DEFAULT_ACTION,
     "-rwr-numlast <int>          : N to use in \"last-N\" and \"last-but-N\" scorers"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void
stochasticRelaxationRadiosityParseOptions(int *argc, char **argv) {
    parseGeneralOptions(srrOptions, argc, argv);
}

void
randomWalkRadiosityParseOptions(int *argc, char **argv) {
    parseGeneralOptions(rwrOptions, argc, argv);
}

static ENUMDESC globalRayMatterPixelFilters[] = {
    {RayMatterFilterType::BOX_FILTER, "box", 2},
    {RayMatterFilterType::TENT_FILTER, "tent", 2},
    {RayMatterFilterType::GAUSS_FILTER, "gaussian 1/sqrt2", 2},
    {RayMatterFilterType::GAUSS2_FILTER, "gaussian 1/2", 2},
    {0, nullptr, 0}
};
MakeEnumOptTypeStruct(rmPixelFilterTypeStruct, globalRayMatterPixelFilters);

static CommandLineOptionDescription globalRayMatterOptions[] =
{
    {"-rm-samples-per-pixel", 6, &GLOBAL_options_intType, &GLOBAL_rayCasting_rayMatterState.samplesPerPixel, DEFAULT_ACTION,
     "-rm-samples-per-pixel <number>\t: eye-rays per pixel"},
    {"-rm-pixel-filter", 7, &rmPixelFilterTypeStruct, &GLOBAL_rayCasting_rayMatterState.filter, DEFAULT_ACTION,
     "-rm-pixel-filter <type>\t: Select filter - \"box\", \"tent\", \"gaussian 1/sqrt2\", \"gaussian 1/2\""},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void
rayMattingParseOptions(int *argc, char **argv) {
    parseGeneralOptions(globalRayMatterOptions, argc, argv);
}

/*** Enum Option types ***/

static ENUMDESC globalRadModeValues[] = {
    {STORED_NONE, "none", 2},
    {STORED_DIRECT, "direct", 2},
    {STORED_INDIRECT, "indirect", 2},
    {STORED_PHOTON_MAP, "photonmap", 2},
    {0, nullptr, 0}
};

MakeEnumOptTypeStruct(radModeTypeStruct, globalRadModeValues);
#define TradMode (&radModeTypeStruct)

static ENUMDESC globalLightModeValues[] = {
        {POWER_LIGHTS,     "power",     2},
        {IMPORTANT_LIGHTS, "important", 2},
        {ALL_LIGHTS,       "all",       2},
        {0, nullptr,                       0}
};
MakeEnumOptTypeStruct(lightModeTypeStruct, globalLightModeValues);
#define TlightMode (&lightModeTypeStruct)

static ENUMDESC globalSamplingModeValues[] = {
    {BRDF_SAMPLING, "bsdf", 2},
    {CLASSICAL_SAMPLING, "classical", 2},
    {0, nullptr, 0}
};
MakeEnumOptTypeStruct(samplingModeTypeStruct, globalSamplingModeValues);
#define TsamplingMode (&samplingModeTypeStruct)

static CommandLineOptionDescription globalStochasticRatTracerOptions[] = {
    {"-rts-samples-per-pixel", 7, &GLOBAL_options_intType, &GLOBAL_raytracing_state.samplesPerPixel, DEFAULT_ACTION,
     "-rts-samples-per-pixel <number>\t: eye-rays per pixel"},
    {"-rts-no-progressive", 9, Tsetfalse, &GLOBAL_raytracing_state.progressiveTracing, DEFAULT_ACTION,
     "-rts-no-progressive\t: don't do progressive image refinement"},
    {"-rts-rad-mode", 8, TradMode, &GLOBAL_raytracing_state.radMode, DEFAULT_ACTION,
     "-rts-rad-mode <type>\t: Stored radiance usage - \"none\", \"direct\", \"indirect\", \"photonmap\""},
    {"-rts-no-lightsampling", 9, Tsetfalse, &GLOBAL_raytracing_state.nextEvent, DEFAULT_ACTION,
     "-rts-no-lightsampling\t: don't do explicit light sampling"},
    {"-rts-l-mode", 8, TlightMode, &GLOBAL_raytracing_state.lightMode, DEFAULT_ACTION,
     "-rts-l-mode <type>\t: Light sampling mode - \"power\", \"important\", \"all\""},
    {"-rts-l-samples", 8, &GLOBAL_options_intType, &GLOBAL_raytracing_state.nextEventSamples, DEFAULT_ACTION,
     "-rts-l-samples <number>\t: explicit light source samples at each hit"},
    {"-rts-scatter-samples", 7, &GLOBAL_options_intType, &GLOBAL_raytracing_state.scatterSamples, DEFAULT_ACTION,
     "-rts-scatter-samples <number>\t: scattered rays at each bounce"},
    {"-rts-do-fdg", 0, Tsettrue, &GLOBAL_raytracing_state.differentFirstDG, DEFAULT_ACTION,
     "-rts-do-fdg\t: use different nr. of scatter samples for first diffuse/glossy bounce"},
    {"-rts-fdg-samples", 8, &GLOBAL_options_intType, &GLOBAL_raytracing_state.firstDGSamples, DEFAULT_ACTION,
     "-rts-fdg-samples <number>\t: scattered rays at first diffuse/glossy bounce"},
    {"-rts-separate-specular", 8, Tsettrue, &GLOBAL_raytracing_state.separateSpecular, DEFAULT_ACTION,
     "-rts-separate-specular\t: always shoot separate rays for specular scattering"},
    {"-rts-s-mode", 9, TsamplingMode, &GLOBAL_raytracing_state.reflectionSampling, DEFAULT_ACTION,
     "-rts-s-mode <type>\t: Sampling mode - \"bsdf\", \"classical\""},
    {"-rts-min-path-length", 8, &GLOBAL_options_intType, &GLOBAL_raytracing_state.minPathDepth, DEFAULT_ACTION,
     "-rts-min-path-length <number>\t: minimum path length before Russian roulette"},
    {"-rts-max-path-length", 8, &GLOBAL_options_intType, &GLOBAL_raytracing_state.maxPathDepth, DEFAULT_ACTION,
     "-rts-max-path-length <number>\t: maximum path length (ignoring higher orders)"},
    {"-rts-NOdirect-background-rad", 8, Tsetfalse, &GLOBAL_raytracing_state.backgroundDirect, DEFAULT_ACTION,
     "-rts-NOdirect-background-rad\t: patchIsOnOmitSet direct background radiance."},
    {"-rts-NOindirect-background-rad", 8, Tsetfalse, &GLOBAL_raytracing_state.backgroundIndirect, DEFAULT_ACTION,
     "-rts-NOindirect-background-rad\t: patchIsOnOmitSet indirect background radiance."},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void
stochasticRayTracerParseOptions(int *argc, char **argv) {
    parseGeneralOptions(globalStochasticRatTracerOptions, argc, argv);
}

MakeNStringTypeStruct(RegExpStringType, MAX_REGEXP_SIZE);

static CommandLineOptionDescription globalBiDirectionalOptions[] = {
    {"-bidir-samples-per-pixel", 8, &GLOBAL_options_intType, &GLOBAL_rayTracing_biDirectionalPath.baseConfig.samplesPerPixel, DEFAULT_ACTION,
    "-bidir-samples-per-pixel <number> : eye-rays per pixel"},
    {"-bidir-no-progressive", 11, Tsetfalse, &GLOBAL_rayTracing_biDirectionalPath.baseConfig.progressiveTracing, DEFAULT_ACTION,
    "-bidir-no-progressive          \t: don't do progressive image refinement"},
    {"-bidir-max-eye-path-length", 12, &GLOBAL_options_intType, &GLOBAL_rayTracing_biDirectionalPath.baseConfig.maximumEyePathDepth, DEFAULT_ACTION,
    "-bidir-max-eye-path-length <number>: maximum eye path length"},
    {"-bidir-max-light-path-length", 12, &GLOBAL_options_intType, &GLOBAL_rayTracing_biDirectionalPath.baseConfig.maximumLightPathDepth, DEFAULT_ACTION,
    "-bidir-max-light-path-length <number>: maximum light path length"},
    {"-bidir-max-path-length", 12, &GLOBAL_options_intType, &GLOBAL_rayTracing_biDirectionalPath.baseConfig.maximumPathDepth, DEFAULT_ACTION,
    "-bidir-max-path-length <number>\t: maximum combined path length"},
    {"-bidir-min-path-length", 12, &GLOBAL_options_intType, &GLOBAL_rayTracing_biDirectionalPath.baseConfig.minimumPathDepth, DEFAULT_ACTION,
    "-bidir-min-path-length <number>\t: minimum path length before russian roulette"},
    {"-bidir-no-light-importance", 11, Tsetfalse, &GLOBAL_rayTracing_biDirectionalPath.baseConfig.sampleImportantLights, DEFAULT_ACTION,
    "-bidir-no-light-importance     \t: sample lights based on power, ignoring their importance"},
    {"-bidir-use-regexp", 12, Tsettrue, &GLOBAL_rayTracing_biDirectionalPath.baseConfig.useSpars, DEFAULT_ACTION,
    "-bidir-use-regexp\t: use regular expressions for path evaluation"},
    {"-bidir-use-emitted", 12, Tbool, &GLOBAL_rayTracing_biDirectionalPath.baseConfig.doLe, DEFAULT_ACTION,
    "-bidir-use-emitted <yes|no>\t: use reg exp for emitted radiance"},
    {"-bidir-rexp-emitted", 13, &RegExpStringType, GLOBAL_rayTracing_biDirectionalPath.baseConfig.leRegExp, DEFAULT_ACTION,
    "-bidir-rexp-emitted <string>\t: reg exp for emitted radiance"},
    {"-bidir-reg-direct", 12, Tbool, &GLOBAL_rayTracing_biDirectionalPath.baseConfig.doLD, DEFAULT_ACTION,
    "-bidir-reg-direct <yes|no>\t: use reg exp for stored direct illumination (galerkin!)"},
    {"-bidir-rexp-direct", 13, &RegExpStringType, GLOBAL_rayTracing_biDirectionalPath.baseConfig.ldRegExp, DEFAULT_ACTION,
    "-bidir-rexp-direct <string>\t: reg exp for stored direct illumination"},
    {"-bidir-reg-indirect", 12, Tbool, &GLOBAL_rayTracing_biDirectionalPath.baseConfig.doLI, DEFAULT_ACTION,
    "-bidir-reg-indirect <yes|no>\t: use reg exp for stored indirect illumination (galerkin!)"},
    {"-bidir-rexp-indirect", 13, &RegExpStringType, GLOBAL_rayTracing_biDirectionalPath.baseConfig.liRegExp, DEFAULT_ACTION,
    "-bidir-rexp-indirect <string>\t: reg exp for stored indirect illumination"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void
biDirectionalPathParseOptions(int *argc, char **argv) {
    parseGeneralOptions(globalBiDirectionalOptions, argc, argv);
}

static char *globalRaytracingMethodsString;
static char *globalRayTracerName;

static void
mainRayTracingOption(void *value) {
    const char *name = *(char **) value;
    strcpy(globalRayTracerName, name);
}

static CommandLineOptionDescription globalRaytracingOptions[] = {
    {"-raytracing-method", 4, Tstring,  nullptr, mainRayTracingOption, globalRaytracingMethodsString},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void
rayTracingParseOptions(
    int *argc,
    char **argv,
    char raytracingMethodsString[],
    char *rayTracerName) {
    globalRayTracerName = rayTracerName;
    globalRaytracingMethodsString = raytracingMethodsString;
    parseGeneralOptions(globalRaytracingOptions, argc, argv);
}

// Command line options
static CommandLineOptionDescription globalPhotonMapOptions[] = {
    {"-pmap-do-global", 9, Tbool, &GLOBAL_photonMap_state.doGlobalMap, DEFAULT_ACTION,
     "-pmap-do-global <true|false> : Trace photons for the global map"},
    {"-pmap-global-paths", 9, &GLOBAL_options_intType, &GLOBAL_photonMap_state.gPathsPerIteration, DEFAULT_ACTION,
     "-pmap-global-paths <number> : Number of paths per iteration for the global map"},
    {"-pmap-g-preirradiance", 11, Tbool, &GLOBAL_photonMap_state.precomputeGIrradiance, DEFAULT_ACTION,
     "-pmap-g-preirradiance <true|false> : Use irradiance precomputation for global map"},
    {"-pmap-do-caustic", 9, Tbool, &GLOBAL_photonMap_state.doCausticMap, DEFAULT_ACTION,
     "-pmap-do-caustic <true|false> : Trace photons for the caustic map"},
    {"-pmap-caustic-paths", 9,  &GLOBAL_options_intType, &GLOBAL_photonMap_state.cPathsPerIteration, DEFAULT_ACTION,
     "-pmap-caustic-paths <number> : Number of paths per iteration for the caustic map"},
    {"-pmap-render-hits", 9, Tsettrue, &GLOBAL_photonMap_state.renderImage, DEFAULT_ACTION,
     "-pmap-render-hits: Show photon hits on screen"},
    {"-pmap-recon-gphotons", 9, &GLOBAL_options_intType, &GLOBAL_photonMap_state.reconGPhotons, DEFAULT_ACTION,
     "-pmap-recon-cphotons <number> : Number of photons to use in reconstructions (global map)"},
    {"-pmap-recon-iphotons", 9, &GLOBAL_options_intType, &GLOBAL_photonMap_state.reconCPhotons, DEFAULT_ACTION,
     "-pmap-recon-photons <number> : Number of photons to use in reconstructions (caustic map)"},
    {"-pmap-recon-photons", 9, &GLOBAL_options_intType, &GLOBAL_photonMap_state.reconIPhotons, DEFAULT_ACTION,
     "-pmap-recon-photons <number> : Number of photons to use in reconstructions (importance)"},
    {"-pmap-balancing", 9, Tbool, &GLOBAL_photonMap_state.balanceKDTree, DEFAULT_ACTION,
     "-pmap-balancing <true|false> : Balance KD Tree before raytracing"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void
photonMapParseOptions(int *argc, char **argv) {
    parseGeneralOptions(globalPhotonMapOptions, argc, argv);
}

#endif
