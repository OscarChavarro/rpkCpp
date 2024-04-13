/**
Monte Carlo Radiosity: common code for stochastic relaxation and random walks
*/

#include <cstdlib>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/mymath.h"
#include "material/statistics.h"
#include "skin/Patch.h"
#include "skin/Vertex.h"
#include "scene/scene.h"
#include "common/options.h"
#include "render/potential.h"
#include "scene/Camera.h"
#include "render/render.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "raycasting/stochasticRaytracing/mcradP.h"

STATE GLOBAL_stochasticRaytracing_monteCarloRadiosityState;

static ENUMDESC globalApproximateValues[] = {
    {AT_CONSTANT, "constant", 2},
    {AT_LINEAR, "linear", 2},
    {AT_QUADRATIC, "quadratic", 2},
    {AT_CUBIC, "cubic", 2},
    {0, nullptr, 0}
};

MakeEnumOptTypeStruct(approxTypeStruct, globalApproximateValues);
#define Tapprox (&approxTypeStruct)

static ENUMDESC clusteringVals[] = {
    {NO_CLUSTERING, "none",      2},
    {ISOTROPIC_CLUSTERING, "isotropic", 2},
    {ORIENTED_CLUSTERING, "oriented",  2},
    {0, nullptr, 0}
};
MakeEnumOptTypeStruct(clusteringTypeStruct, clusteringVals);
#define Tclustering (&clusteringTypeStruct)

static ENUMDESC sequenceVals[] = {
    {S4D_RANDOM, "PseudoRandom", 2},
    {S4D_HALTON, "Halton", 2},
    {S4D_NIEDERREITER, "Niederreiter", 2},
    {0, nullptr, 0}
};
MakeEnumOptTypeStruct(sequenceTypeStruct, sequenceVals);
#define Tsequence (&sequenceTypeStruct)

static ENUMDESC estTypeVals[] = {
    {RW_SHOOTING, "Shooting", 2},
    {RW_GATHERING, "Gathering", 2},
    {0, nullptr, 0}
};
MakeEnumOptTypeStruct(estTypeTypeStruct, estTypeVals);
#define TestType (&estTypeTypeStruct)

static ENUMDESC globalEstKindValues[] = {
    {RW_COLLISION, "Collision", 2},
    {RW_ABSORPTION, "Absorption", 2},
    {RW_SURVIVAL, "Survival", 2},
    {RW_LAST_BUT_NTH, "Last-but-N", 2},
    {RW_N_LAST, "Last-N", 2},
    {0, nullptr, 0}
};
MakeEnumOptTypeStruct(estKindTypeStruct, globalEstKindValues);
#define TestKind (&estKindTypeStruct)

static ENUMDESC showWhatVals[] = {
    {SHOW_TOTAL_RADIANCE, "total-radiance", 2},
    {SHOW_INDIRECT_RADIANCE, "indirect-radiance", 2},
    {SHOW_IMPORTANCE, "importance", 2},
    {0, nullptr, 0}
};
MakeEnumOptTypeStruct(showWhatTypeStruct, showWhatVals);
#define TshowWhat (&showWhatTypeStruct)

static CommandLineOptionDescription srrOptions[] = {
    {"-srr-ray-units", 8, &GLOBAL_options_intType, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.rayUnitsPerIt,                                  DEFAULT_ACTION,
     "-srr-ray-units <n>          : To tune the amount of work in a single iteration"},
    {"-srr-bidirectional", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.bidirectionalTransfers,                    DEFAULT_ACTION,
     "-srr-bidirectional <yes|no> : Use lines bidirectionally"},
    {"-srr-control-variate", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate,                  DEFAULT_ACTION,
     "-srr-control-variate <y|n>  : Constant Control Variate variance reduction"},
    {"-srr-indirect-only", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly,                              DEFAULT_ACTION,
     "-srr-indirect-only <y|n>    : Compute indirect illumination only"},
    {"-srr-importance-driven", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven, DEFAULT_ACTION,
     "-srr-importance-driven <y|n>: Use view-importance"},
    {"-srr-sampling-sequence", 7, Tsequence, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sequence,                          DEFAULT_ACTION,
     "-srr-sampling-sequence <type>: \"PseudoRandom\", \"Niederreiter\""},
    {"-srr-approximation", 7, Tapprox, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType, DEFAULT_ACTION,
     "-srr-approximation <order>  : \"constant\", \"linear\", \"quadratic\", \"cubic\""},
    {"-srr-hierarchical", 7, Tbool, &GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing,   DEFAULT_ACTION,
     "-srr-hierarchical <y|n>     : hierarchical refinement"},
    {"-srr-clustering", 7, Tclustering, &GLOBAL_stochasticRaytracing_hierarchy.clustering, DEFAULT_ACTION,
     "-srr-clustering <mode>      : \"none\", \"isotropic\", \"oriented\""},
    {"-srr-epsilon", 7, Tfloat, &GLOBAL_stochasticRaytracing_hierarchy.epsilon,            DEFAULT_ACTION,
     "-srr-epsilon <float>        : link power threshold (relative w.r.t. max. selfemitted power)"},
    {"-srr-minarea", 7, Tfloat, &GLOBAL_stochasticRaytracing_hierarchy.minimumArea, DEFAULT_ACTION,
     "-srr-minarea <float>        : minimal element area (relative w.r.t. total area)"},
    {"-srr-display", 7, TshowWhat, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show,                                        DEFAULT_ACTION,
     "-srr-display <what>         : \"total-radiance\", \"indirect-radiance\", \"weighting-gain\", \"importance\""},
    {"-srr-discard-incremental", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.discardIncremental, DEFAULT_ACTION,
     "-srr-discard-incremenal <y|n>: Discard result of first iteration (incremental steps)"},
    {"-srr-incremental-uses-importance", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.incrementalUsesImportance, DEFAULT_ACTION,
     "-srr-incremental-uses-importance <y|n>: Use view-importance sampling already for the first iteration (incremental steps)"},
    {"-srr-naive-merging", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.naiveMerging, DEFAULT_ACTION,
     "-srr-naive-merging <y|n>    : disable intelligent merging heuristic"},
    {"-srr-nondiffuse-first-shot", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.doNonDiffuseFirstShot,             DEFAULT_ACTION,
     "-srr-nondiffuse-first-shot <y|n>: Do Non-diffuse first shot before real work"},
    {"-srr-initial-ls-samples", 7, &GLOBAL_options_intType, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialLightSourceSamples,             DEFAULT_ACTION,
     "-srr-initial-ls-samples <int>        : nr of samples per light source for initial shot"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

static CommandLineOptionDescription rwrOptions[] = {
    {"-rwr-ray-units", 8, &GLOBAL_options_intType, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.rayUnitsPerIt,                 DEFAULT_ACTION,
     "-rwr-ray-units <n>          : To tune the amount of work in a single iteration"},
    {"-rwr-continuous", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk, DEFAULT_ACTION,
     "-rwr-continuous <y|n>       : Continuous (yes) or Discrete (no) random walk"},
    {"-rwr-control-variate", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate, DEFAULT_ACTION,
     "-rwr-control-variate <y|n>  : Constant Control Variate variance reduction"},
    {"-rwr-indirect-only", 7, Tbool, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly,             DEFAULT_ACTION,
     "-rwr-indirect-only <y|n>    : Compute indirect illumination only"},
    {"-rwr-sampling-sequence", 7, Tsequence, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sequence,         DEFAULT_ACTION,
     "-rwr-sampling-sequence <type>: \"PseudoRandom\", \"Halton\", \"Niederreiter\""},
    {"-rwr-approximation", 7, Tapprox, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType, DEFAULT_ACTION,
     "-rwr-approximation <order>  : \"constant\", \"linear\", \"quadratic\", \"cubic\""},
    {"-rwr-estimator", 7, TestType, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorType, DEFAULT_ACTION,
     "-rwr-estimator <type>       : \"shooting\", \"gathering\""},
    {"-rwr-score", 7, TestKind, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorKind, DEFAULT_ACTION,
     "-rwr-score <kind>           : \"collision\", \"absorption\", \"survival\", \"last-N\", \"last-but-N\""},
    {"-rwr-numlast", 12, &GLOBAL_options_intType, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkNumLast,              DEFAULT_ACTION,
     "-rwr-numlast <int>          : N to use in \"last-N\" and \"last-but-N\" scorers"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

/**
Common routines for stochastic relaxation and random walks
*/
void
monteCarloRadiosityDefaults() {
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.rayUnitsPerIt = 10;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.bidirectionalTransfers = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance.clear();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sequence = S4D_NIEDERREITER;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType = AT_CONSTANT;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.radianceDriven = true;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdated = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdatedFromScratch = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorType = RW_SHOOTING;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorKind = RW_COLLISION;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkNumLast = 1;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.discardIncremental = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.incrementalUsesImportance = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.naiveMerging = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show = SHOW_TOTAL_RADIANCE;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.doNonDiffuseFirstShot = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialLightSourceSamples = 1000;

    elementHierarchyDefaults();
    monteCarloRadiosityInitBasis();
}

void
stochasticRelaxationRadiosityParseOptions(int *argc, char **argv) {
    parseGeneralOptions(srrOptions, argc, argv);
}

void
randomWalkRadiosityParseOptions(int *argc, char **argv) {
    parseGeneralOptions(rwrOptions, argc, argv);
}

/**
For counting how much CPU time was used for the computations
*/
void
monteCarloRadiosityUpdateCpuSecs() {
    clock_t t;

    t = clock();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds += (float) (t - GLOBAL_stochasticRaytracing_monteCarloRadiosityState.lastClock) / (float) CLOCKS_PER_SEC;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.lastClock = t;
}

Element *
monteCarloRadiosityCreatePatchData(Patch *patch) {
    patch->radianceData = stochasticRadiosityElementCreateFromPatch(patch);
    return patch->radianceData;
}

void
monteCarloRadiosityDestroyPatchData(Patch *patch) {
    if ( patch->radianceData ) {
        stochasticRadiosityElementDestroy(topLevelGalerkinElement(patch));
    }
    patch->radianceData = nullptr;
}

/**
Compute new color for the patch: fine if no hierarchical refinement is used, e.g.
in the current random walk radiosity implementation
*/
void
monteCarloRadiosityPatchComputeNewColor(Patch *patch) {
    patch->color = stochasticRadiosityElementColor(topLevelGalerkinElement(patch));
    patch->computeVertexColors();
}

/**
Initializes the computations for the current scene (if any): initialisations
are delayed to just before the first iteration step, see ReInit() below
*/
void
monteCarloRadiosityInit() {
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited = false;
}

/**
Initialises patch data
*/
static void
monteCarloRadiosityInitPatch(Patch *patch) {
    ColorRgb Ed = topLevelGalerkinElement(patch)->Ed;

    reAllocCoefficients(topLevelGalerkinElement(patch));
    stochasticRadiosityClearCoefficients(getTopLevelPatchRad(patch), getTopLevelPatchBasis(patch));
    stochasticRadiosityClearCoefficients(getTopLevelPatchUnShotRad(patch), getTopLevelPatchBasis(patch));
    stochasticRadiosityClearCoefficients(getTopLevelPatchReceivedRad(patch), getTopLevelPatchBasis(patch));

    getTopLevelPatchRad(patch)[0] = getTopLevelPatchUnShotRad(patch)[0] = topLevelGalerkinElement(patch)->sourceRad = Ed;
    getTopLevelPatchReceivedRad(patch)[0].clear();

    topLevelGalerkinElement(patch)->rayIndex = patch->id * 11;
    topLevelGalerkinElement(patch)->quality = 0.0;
    topLevelGalerkinElement(patch)->ng = 0;
    topLevelGalerkinElement(patch)->importance = 0.0;
    topLevelGalerkinElement(patch)->unShotImportance = 0.0;
    topLevelGalerkinElement(patch)->receivedImportance = 0.0;
    topLevelGalerkinElement(patch)->sourceImportance = 0.0;
}

/**
Routines below update/re-initialise importance after a viewing change
*/
static void
monteCarloRadiosityPullImportances(Element *element) {
    StochasticRadiosityElement *child = (StochasticRadiosityElement *)element;
    StochasticRadiosityElement *parent = (StochasticRadiosityElement *)child->parent;
    stochasticRadiosityElementPullImportance(parent, child, &parent->importance, &child->importance);
    stochasticRadiosityElementPullImportance(parent, child, &parent->sourceImportance, &child->sourceImportance);
    stochasticRadiosityElementPullImportance(parent, child, &parent->unShotImportance, &child->unShotImportance);
}

static void
monteCarloRadiosityAccumulateImportances(StochasticRadiosityElement *elem) {
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp += elem->area * elem->importance;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp += elem->area * elem->sourceImportance;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp += elem->area * std::fabs(elem->unShotImportance);
}

/**
Update importance in the element hierarchy starting with the top cluster
*/
static void
monteCarloRadiosityUpdateImportance(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = (StochasticRadiosityElement *)element;

    if ( stochasticRadiosityElement == nullptr ) {
        return;
    }

    if ( !stochasticRadiosityElement->traverseAllChildren(monteCarloRadiosityUpdateImportance) ) {
        // Leaf element
        float delta_imp = (float)(stochasticRadiosityElement->patch->isVisible() ? 1.0 : 0.0) - stochasticRadiosityElement->sourceImportance;
        stochasticRadiosityElement->importance += delta_imp;
        stochasticRadiosityElement->sourceImportance += delta_imp;
        stochasticRadiosityElement->unShotImportance += delta_imp;
        monteCarloRadiosityAccumulateImportances(stochasticRadiosityElement);
    } else {
        // Not a leaf element: clear & pull importance
        stochasticRadiosityElement->importance = stochasticRadiosityElement->sourceImportance = stochasticRadiosityElement->unShotImportance = 0.0;
        stochasticRadiosityElement->traverseAllChildren(monteCarloRadiosityPullImportances);
    }
}

/**
Re-init importance in the element hierarchy starting with the top cluster
*/
static void
monteCarloRadiosityReInitImportance(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = (StochasticRadiosityElement *)element;

    if ( stochasticRadiosityElement == nullptr ) {
        return;
    }

    if ( !stochasticRadiosityElement->traverseAllChildren(monteCarloRadiosityReInitImportance) ) {
        // Leaf element
        stochasticRadiosityElement->importance = (stochasticRadiosityElement->patch->isVisible()) ? 1.0 : 0.0;
        stochasticRadiosityElement->sourceImportance = stochasticRadiosityElement->importance;
        stochasticRadiosityElement->unShotImportance = stochasticRadiosityElement->importance;
        monteCarloRadiosityAccumulateImportances(stochasticRadiosityElement);
    } else {
        // Not a leaf element: clear & pull importance
        stochasticRadiosityElement->importance = stochasticRadiosityElement->sourceImportance = stochasticRadiosityElement->unShotImportance = 0.0;
        stochasticRadiosityElement->traverseAllChildren(monteCarloRadiosityPullImportances);
    }
}

void
monteCarloRadiosityUpdateViewImportance(java::ArrayList<Patch *> *scenePatches) {
    fprintf(stderr, "Updating direct visibility ... \n");

    updateDirectVisibility(scenePatches);

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp = 0.0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp = 0.0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp = 0.0;
    monteCarloRadiosityUpdateImportance(GLOBAL_stochasticRaytracing_hierarchy.topCluster);

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp < GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp ) {
        fprintf(stderr, "Importance will be recomputed incrementally.\n");
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdatedFromScratch = false;
    } else {
        fprintf(stderr, "Importance will be recomputed from scratch.\n");
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdatedFromScratch = true;

        // Re-compute from scratch
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp = 0.0;
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp = 0.0;
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp = 0.0;
        monteCarloRadiosityReInitImportance(GLOBAL_stochasticRaytracing_hierarchy.topCluster);
    }

    GLOBAL_camera_mainCamera.changed = false; // Indicate that direct importance has been computed for this view already
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays = 0; // Start over
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdated = true;
}

/**
Computes max_i (A_T/A_i): the ratio of the total area over the minimal patch
area in the scene, ignoring the 10% area occupied by the smallest patches
*/
static double
monteCarloRadiosityDetermineAreaFraction(
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Geometry *> *sceneGeometries)
{
    float *areas;
    float cumul;
    float areaFrac;
    int numberOfPatchIds = Patch::getNextId();
    int i;

    if ( sceneGeometries == nullptr || sceneGeometries->size() == 0 ) {
        // An arbitrary positive number (in order to avoid divisions by zero
        return 100;
    }

    // Build a table of patch areas
    areas = (float *)malloc(numberOfPatchIds * sizeof(float));
    for ( i = 0; i < numberOfPatchIds; i++ ) {
        areas[i] = 0.0;
    }
    for ( i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        areas[patch->id] = patch->area;
    }

    // Sort the table to decreasing areas
    qsort((void *) areas,
        numberOfPatchIds,
        sizeof(float),
        (QSORT_CALLBACK_TYPE) floatCompare);

    // Find the patch such that 10% of the total surface area is filled by smaller patches
    for ( i = numberOfPatchIds - 1, cumul = 0.0; i >= 0 && cumul < GLOBAL_statistics.totalArea * 0.1; i-- ) {
        cumul += areas[i];
    }
    areaFrac = (i >= 0 && areas[i] > 0.0) ? GLOBAL_statistics.totalArea / areas[i] : (float)GLOBAL_statistics.numberOfPatches;

    free(areas);

    return areaFrac;
}

/**
Determines elementary ray power for the initial incremental iterations
*/
static void
monteCarloRadiosityDetermineInitialNrRays(
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Geometry *> *sceneGeometries)
{
    double areaFrac = monteCarloRadiosityDetermineAreaFraction(scenePatches, sceneGeometries);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays = (long) ((double) GLOBAL_stochasticRaytracing_monteCarloRadiosityState.rayUnitsPerIt * areaFrac);
}

/**
Really initialises: before the first iteration step
*/
void
monteCarloRadiosityReInit(
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Geometry *> *sceneGeometries)
{
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited ) {
        return;
    }

    fprintf(stderr, "Initialising Monte Carlo radiosity ...\n");

    setSequence4D(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sequence);

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited = true;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds = 0.0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.lastClock = clock();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration = 0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevTracedRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.numberOfMisses = 0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevImportanceTracedRays = 0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.setSource = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedPaths = 0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance.clear();

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.clear();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp = 0.0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.clear();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp = 0.0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux.clear();
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        monteCarloRadiosityInitPatch(patch);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.addScaled(
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux,
            M_PI * patch->area,
            getTopLevelPatchUnShotRad(patch)[0]);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.addScaled(
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux,
            M_PI * patch->area,
            getTopLevelPatchRad(patch)[0]);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux.addScaled(
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux,
            M_PI * patch->area *
            (topLevelGalerkinElement(patch)->importance - topLevelGalerkinElement(patch)->sourceImportance),
            getTopLevelPatchUnShotRad(patch)[0]);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp += patch->area * std::fabs(topLevelGalerkinElement(patch)->unShotImportance);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp += patch->area * topLevelGalerkinElement(patch)->importance;
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp += patch->area * topLevelGalerkinElement(patch)->sourceImportance;
        monteCarloRadiosityPatchComputeNewColor(patch);
    }

    monteCarloRadiosityDetermineInitialNrRays(scenePatches, sceneGeometries);

    elementHierarchyInit();

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
        monteCarloRadiosityUpdateViewImportance(scenePatches);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdatedFromScratch = true;
    }
}

void
monteCarloRadiosityPreStep(java::ArrayList<Patch *> *scenePatches, java::ArrayList<Geometry *> *sceneGeometries) {
    if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited ) {
        monteCarloRadiosityReInit(scenePatches, sceneGeometries);
    }
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven && GLOBAL_camera_mainCamera.changed ) {
        monteCarloRadiosityUpdateViewImportance(scenePatches);
    }

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.lastClock = clock();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration++;
}

/**
Undoes the effect of mainInit() and all side-effects of Step()
*/
void
monteCarloRadiosityTerminate(java::ArrayList<Patch *> *scenePatches) {
    elementHierarchyTerminate(scenePatches);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited = false;
}

static ColorRgb
monteCarloRadiosityDiffuseReflectanceAtPoint(Patch *patch, double u, double v) {
    RayHit hit;
    Vector3D point;
    patch->uniformPoint(u, v, &point);
    hitInit(&hit, patch, nullptr, &point, &patch->normal, patch->surface->material, 0.0);
    hit.uv.u = u;
    hit.uv.v = v;
    hit.flags |= HIT_UV;
    return bsdfScatteredPower(hit.material->bsdf, &hit, &patch->normal, BRDF_DIFFUSE_COMPONENT);
}

static ColorRgb
vertexReflectance(Vertex *v) {
    int count = 0;
    ColorRgb rd;

    rd.clear();
    for ( int i = 0; v->radianceData != nullptr && i < v->radianceData->size(); i++ ) {
        Element *genericElement = v->radianceData->get(i);
        if ( genericElement->className != ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY ) {
            continue;
        }
        StochasticRadiosityElement *element = (StochasticRadiosityElement *)genericElement;
        if ( !element->regularSubElements ) {
            rd.add(rd, element->Rd);
            count++;
        }
    }

    if ( count > 0 ) {
        rd.scaleInverse((float) count, rd);
    }

    return rd;
}

static ColorRgb
monteCarloRadiosityInterpolatedReflectanceAtPoint(StochasticRadiosityElement *leaf, double u, double v) {
    static StochasticRadiosityElement *cachedleaf = nullptr;
    static ColorRgb vrd[4], rd;

    if ( leaf != cachedleaf ) {
        int i;
        for ( i = 0; i < leaf->numberOfVertices; i++ ) {
            vrd[i] = vertexReflectance(leaf->vertices[i]);
        }
    }
    cachedleaf = leaf;

    rd.clear();
    switch ( leaf->numberOfVertices ) {
        case 3:
            rd.interpolateBarycentric(vrd[0], vrd[1], vrd[2], (float)u, (float)v);
            break;
        case 4:
            rd.interpolateBiLinear(vrd[0], vrd[1], vrd[2], vrd[3], (float) u, (float) v);
            break;
        default:
            logFatal(-1, "monteCarloRadiosityInterpolatedReflectanceAtPoint", "Invalid nr of vertices %d", leaf->numberOfVertices);
    }
    return rd;
}

/**
Returns the radiance emitted from the patch at the point with parameters
(u,v) into the direction 'dir'
*/
ColorRgb
monteCarloRadiosityGetRadiance(Patch *patch, double u, double v, Vector3D /*dir*/) {
    ColorRgb TrueRdAtPoint = monteCarloRadiosityDiffuseReflectanceAtPoint(patch, u, v);
    StochasticRadiosityElement *leaf = stochasticRadiosityElementRegularLeafElementAtPoint(
            topLevelGalerkinElement(patch), &u, &v);
    ColorRgb UsedRdAtPoint = GLOBAL_render_renderOptions.smoothShading ? monteCarloRadiosityInterpolatedReflectanceAtPoint(leaf, u, v) : leaf->Rd;
    ColorRgb radianceAtPoint = stochasticRadiosityElementDisplayRadianceAtPoint(leaf, u, v);
    ColorRgb sourceRad;
    sourceRad.clear();

    // Subtract source radiance
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show != SHOW_INDIRECT_RADIANCE ) {
        // source_rad is self-emitted radiance if !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly. It is direct
        // illumination if GLOBAL_stochasticRaytracing_monteCarloRadiosityState.direct_only
        if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.doNonDiffuseFirstShot ) {
            sourceRad = leaf->sourceRad;
        }
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly || GLOBAL_stochasticRaytracing_monteCarloRadiosityState.doNonDiffuseFirstShot ) {
            // Subtract self-emitted radiance
            sourceRad.add(sourceRad, leaf->Ed);
        }
    }
    radianceAtPoint.subtract(radianceAtPoint, sourceRad);

    radianceAtPoint.scalarProduct(radianceAtPoint, TrueRdAtPoint);
    radianceAtPoint.divide(radianceAtPoint, UsedRdAtPoint);

    // Re-add source radiance
    radianceAtPoint.add(radianceAtPoint, sourceRad);

    return radianceAtPoint;
}

/**
Returns scalar reflectance, for importance propagation
*/
float
monteCarloRadiosityScalarReflectance(Patch *P) {
    return stochasticRadiosityElementScalarReflectance(topLevelGalerkinElement(P));
}
