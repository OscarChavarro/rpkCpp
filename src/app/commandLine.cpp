#include "common/options.h"
#include "common/RenderOptions.h"
#include "scene/Camera.h"
#include "raycasting/simple/RayMatter.h"
#include "raycasting/raytracing/bidiroptions.h"
#include "raycasting/stochasticRaytracing/basismcrad.h"
#include "raycasting/stochasticRaytracing/HierarchyClusteringMode.h"
#include "raycasting/stochasticRaytracing/sample4d.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
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

void
stochasticRayTracerDefaults() {
    // Normal
    GLOBAL_raytracing_state.samplesPerPixel = 1;
    GLOBAL_raytracing_state.progressiveTracing = true;

    GLOBAL_raytracing_state.doFrameCoherent = false;
    GLOBAL_raytracing_state.doCorrelatedSampling = false;
    GLOBAL_raytracing_state.baseSeed = 0xFE062134;

    GLOBAL_raytracing_state.radMode = STORED_NONE;

    GLOBAL_raytracing_state.nextEvent = true;
    GLOBAL_raytracing_state.nextEventSamples = 1;
    GLOBAL_raytracing_state.lightMode = ALL_LIGHTS;

    GLOBAL_raytracing_state.backgroundDirect = false;
    GLOBAL_raytracing_state.backgroundIndirect = true;
    GLOBAL_raytracing_state.backgroundSampling = false;

    GLOBAL_raytracing_state.scatterSamples = 1;
    GLOBAL_raytracing_state.differentFirstDG = false;
    GLOBAL_raytracing_state.firstDGSamples = 36;
    GLOBAL_raytracing_state.separateSpecular = false;

    GLOBAL_raytracing_state.reflectionSampling = BRDF_SAMPLING;

    GLOBAL_raytracing_state.minPathDepth = 5;
    GLOBAL_raytracing_state.maxPathDepth = 7;

    // Common
    GLOBAL_raytracing_state.lastScreen = nullptr;
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
    {"-bidir-samples-per-pixel", 8, &GLOBAL_options_intType, &GLOBAL_rayTracing_biDirectionalPath.basecfg.samplesPerPixel, DEFAULT_ACTION,
    "-bidir-samples-per-pixel <number> : eye-rays per pixel"},
    {"-bidir-no-progressive", 11, Tsetfalse, &GLOBAL_rayTracing_biDirectionalPath.basecfg.progressiveTracing, DEFAULT_ACTION,
    "-bidir-no-progressive          \t: don't do progressive image refinement"},
    {"-bidir-max-eye-path-length", 12, &GLOBAL_options_intType, &GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumEyePathDepth, DEFAULT_ACTION,
    "-bidir-max-eye-path-length <number>: maximum eye path length"},
    {"-bidir-max-light-path-length", 12, &GLOBAL_options_intType, &GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumLightPathDepth, DEFAULT_ACTION,
    "-bidir-max-light-path-length <number>: maximum light path length"},
    {"-bidir-max-path-length", 12, &GLOBAL_options_intType, &GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumPathDepth, DEFAULT_ACTION,
    "-bidir-max-path-length <number>\t: maximum combined path length"},
    {"-bidir-min-path-length", 12, &GLOBAL_options_intType, &GLOBAL_rayTracing_biDirectionalPath.basecfg.minimumPathDepth, DEFAULT_ACTION,
    "-bidir-min-path-length <number>\t: minimum path length before russian roulette"},
    {"-bidir-no-light-importance", 11, Tsetfalse, &GLOBAL_rayTracing_biDirectionalPath.basecfg.sampleImportantLights, DEFAULT_ACTION,
    "-bidir-no-light-importance     \t: sample lights based on power, ignoring their importance"},
    {"-bidir-use-regexp", 12, Tsettrue, &GLOBAL_rayTracing_biDirectionalPath.basecfg.useSpars, DEFAULT_ACTION,
    "-bidir-use-regexp\t: use regular expressions for path evaluation"},
    {"-bidir-use-emitted", 12, Tbool, &GLOBAL_rayTracing_biDirectionalPath.basecfg.doLe, DEFAULT_ACTION,
    "-bidir-use-emitted <yes|no>\t: use reg exp for emitted radiance"},
    {"-bidir-rexp-emitted", 13, &RegExpStringType, GLOBAL_rayTracing_biDirectionalPath.basecfg.leRegExp, DEFAULT_ACTION,
    "-bidir-rexp-emitted <string>\t: reg exp for emitted radiance"},
    {"-bidir-reg-direct", 12, Tbool, &GLOBAL_rayTracing_biDirectionalPath.basecfg.doLD, DEFAULT_ACTION,
    "-bidir-reg-direct <yes|no>\t: use reg exp for stored direct illumination (galerkin!)"},
    {"-bidir-rexp-direct", 13, &RegExpStringType, GLOBAL_rayTracing_biDirectionalPath.basecfg.ldRegExp, DEFAULT_ACTION,
    "-bidir-rexp-direct <string>\t: reg exp for stored direct illumination"},
    {"-bidir-reg-indirect", 12, Tbool, &GLOBAL_rayTracing_biDirectionalPath.basecfg.doLI, DEFAULT_ACTION,
    "-bidir-reg-indirect <yes|no>\t: use reg exp for stored indirect illumination (galerkin!)"},
    {"-bidir-rexp-indirect", 13, &RegExpStringType, GLOBAL_rayTracing_biDirectionalPath.basecfg.liRegExp, DEFAULT_ACTION,
    "-bidir-rexp-indirect <string>\t: reg exp for stored indirect illumination"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void
biDirectionalPathParseOptions(int *argc, char **argv) {
    parseGeneralOptions(globalBiDirectionalOptions, argc, argv);
}

#endif
