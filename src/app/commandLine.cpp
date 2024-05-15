#include "common/options.h"
#include "scene/Camera.h"
#include "raycasting/stochasticRaytracing/basismcrad.h"
#include "raycasting/stochasticRaytracing/HierarchyClusteringMode.h"
#include "raycasting/stochasticRaytracing/sample4d.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "app/commandLine.h"

#define DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS 4
#define DEFAULT_FORCE_ONE_SIDED true

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
    Vector3D *v = (Vector3D *)val;
    globalCamera.setEyePosition(v->x, v->y, v->z);
}

static void
cameraSetLookPositionOption(void *val) {
    Vector3D *v = (Vector3D *)val;
    globalCamera.setLookPosition(v->x, v->y, v->z);
}

static void
cameraSetUpDirectionOption(void *val) {
    Vector3D *v = (Vector3D *)val;
    globalCamera.setUpDirection(v->x, v->y, v->z);
}

static void cameraSetFieldOfViewOption(void *val) {
    float *v = (float *)val;
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

// Default virtual camera
#define DEFAULT_CAMERA_EYE_POSITION {10.0, 0.0, 0.0}
#define DEFAULT_CAMERA_LOOK_POSITION { 0.0, 0.0, 0.0}
#define DEFAULT_CAMERA_UP_DIRECTION { 0.0, 0.0, 1.0}
#define DEFAULT_CAMERA_FIELD_OF_VIEW 22.5
#define DEFAULT_BACKGROUND_COLOR {0.0, 0.0, 0.0}

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
