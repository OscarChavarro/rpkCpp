/**
Monte Carlo Radiosity: common code for stochastic relaxation and random walks
*/

#include <cstdlib>

#include "common/error.h"
#include "material/statistics.h"
#include "skin/Patch.h"
#include "skin/Vertex.h"
#include "scene/scene.h"
#include "shared/options.h"
#include "shared/potential.h"
#include "shared/camera.h"
#include "shared/render.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/elementmcrad.h"

STATE GLOBAL_stochasticRaytracing_monteCarloRadiosityState;

static ENUMDESC approxVals[] = {
    {AT_CONSTANT, "constant", 2},
    {AT_LINEAR, "linear", 2},
    {AT_QUADRATIC, "quadratic", 2},
    {AT_CUBIC, "cubic", 2},
    {0, nullptr, 0}
};

MakeEnumOptTypeStruct(approxTypeStruct, approxVals);
#define Tapprox (&approxTypeStruct)

static ENUMDESC clusteringVals[] = {
        {NO_CLUSTERING,        "none",      2},
        {ISOTROPIC_CLUSTERING, "isotropic", 2},
        {ORIENTED_CLUSTERING,  "oriented",  2},
        {0, nullptr,                           0}
};
MakeEnumOptTypeStruct(clusteringTypeStruct, clusteringVals);
#define Tclustering (&clusteringTypeStruct)

static ENUMDESC sequenceVals[] = {
        {S4D_RANDOM,       "PseudoRandom", 2},
        {S4D_HALTON,       "Halton",       2},
        {S4D_NIEDERREITER, "Niederreiter", 2},
        {0, nullptr,                          0}
};
MakeEnumOptTypeStruct(sequenceTypeStruct, sequenceVals);
#define Tsequence (&sequenceTypeStruct)

static ENUMDESC estTypeVals[] = {
        {RW_SHOOTING,  "Shooting",  2},
        {RW_GATHERING, "Gathering", 2},
        {0, nullptr,                   0}
};
MakeEnumOptTypeStruct(estTypeTypeStruct, estTypeVals);
#define TestType (&estTypeTypeStruct)

static ENUMDESC estKindVals[] = {
        {RW_COLLISION,    "Collision",  2},
        {RW_ABSORPTION,   "Absorption", 2},
        {RW_SURVIVAL,     "Survival",   2},
        {RW_LAST_BUT_NTH, "Last-but-N", 2},
        {RW_N_LAST,       "Last-N",     2},
        {0,               nullptr,      0}
};
MakeEnumOptTypeStruct(estKindTypeStruct, estKindVals);
#define TestKind (&estKindTypeStruct)

static ENUMDESC showWhatVals[] = {
        {SHOW_TOTAL_RADIANCE,    "total-radiance",    2},
        {SHOW_INDIRECT_RADIANCE, "indirect-radiance", 2},
        {SHOW_IMPORTANCE,        "importance",        2},
        {0, nullptr,                                     0}
};
MakeEnumOptTypeStruct(showWhatTypeStruct, showWhatVals);
#define TshowWhat (&showWhatTypeStruct)

static CMDLINEOPTDESC srrOptions[] = {
    {"-srr-ray-units", 8, Tint, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.rayUnitsPerIt,                                  DEFAULT_ACTION,
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
    {"-srr-minarea", 7, Tfloat, &GLOBAL_stochasticRaytracing_hierarchy.minarea,            DEFAULT_ACTION,
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
    {"-srr-initial-ls-samples", 7, Tint, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialLightSourceSamples,             DEFAULT_ACTION,
     "-srr-initial-ls-samples <int>        : nr of samples per light source for initial shot"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

static CMDLINEOPTDESC rwrOptions[] = {
        {"-rwr-ray-units", 8, Tint, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.rayUnitsPerIt,                 DEFAULT_ACTION,
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
        {"-rwr-numlast", 12, Tint, &GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkNumLast,              DEFAULT_ACTION,
         "-rwr-numlast <int>          : N to use in \"last-N\" and \"last-but-N\" scorers"},
        {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION,
         nullptr}
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
    colorClear(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance);
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
    parseOptions(srrOptions, argc, argv);
}

void
stochasticRelaxationRadiosityPrintOptions(FILE *fp) {
}

void
randomWalkRadiosityParseOptions(int *argc, char **argv) {
    parseOptions(rwrOptions, argc, argv);
}

void
randomWalkRadiosityPrintOptions(FILE *fp) {
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

void *
monteCarloRadiosityCreatePatchData(Patch *patch) {
    patch->radiance_data = (void *) monteCarloRadiosityCreateToplevelSurfaceElement(patch);
    return patch->radiance_data;
}

void
monteCarloRadiosityPrintPatchData(FILE *out, Patch *patch) {
    monteCarloRadiosityPrintElement(out, TOPLEVEL_ELEMENT(patch));
}

void
monteCarloRadiosityDestroyPatchData(Patch *patch) {
    if ( patch->radiance_data ) {
        monteCarloRadiosityDestroyToplevelSurfaceElement(TOPLEVEL_ELEMENT(patch));
    }
    patch->radiance_data = (void *) nullptr;
}

/**
Compute new color for the patch: fine if no hierarchical refinement is used, e.g.
in the current random walk radiosity implementation
*/
void
monteCarloRadiosityPatchComputeNewColor(Patch *patch) {
    patch->color = ElementColor(TOPLEVEL_ELEMENT(patch));
    patchComputeVertexColors(patch);
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
monteCarloRadiosityInitPatch(Patch *P) {
    COLOR Ed = TOPLEVEL_ELEMENT(P)->Ed;

    reAllocCoefficients(TOPLEVEL_ELEMENT(P));
    stochasticRadiosityClearCoefficients(getTopLevelPatchRad(P), getTopLevelPatchBasis(P));
    stochasticRadiosityClearCoefficients(getTopLevelPatchUnShotRad(P), getTopLevelPatchBasis(P));
    stochasticRadiosityClearCoefficients(getTopLevelPatchReceivedRad(P), getTopLevelPatchBasis(P));

    getTopLevelPatchRad(P)[0] = getTopLevelPatchUnShotRad(P)[0] = TOPLEVEL_ELEMENT(P)->source_rad = Ed;
    colorClear(getTopLevelPatchReceivedRad(P)[0]);

    TOPLEVEL_ELEMENT(P)->ray_index = P->id * 11;
    TOPLEVEL_ELEMENT(P)->quality = 0.;
    TOPLEVEL_ELEMENT(P)->ng = 0;

    TOPLEVEL_ELEMENT(P)->imp = TOPLEVEL_ELEMENT(P)->unshot_imp = TOPLEVEL_ELEMENT(P)->received_imp = TOPLEVEL_ELEMENT(P)->source_imp = 0.;
}

/**
Routines below update/re-initialise importance after a viewing change
*/
static void
monteCarloRadiosityPullImportances(ELEMENT *child) {
    ELEMENT *parent = child->parent;
    PullImportance(parent, child, &parent->imp, &child->imp);
    PullImportance(parent, child, &parent->source_imp, &child->source_imp);
    PullImportance(parent, child, &parent->unshot_imp, &child->unshot_imp);
}

static void
monteCarloRadiosityAccumulateImportances(ELEMENT *elem) {
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp += elem->area * elem->imp;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp += elem->area * elem->source_imp;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp += elem->area * fabs(elem->unshot_imp);
}

/**
Update importance in the element hierarchy starting with the top cluster
*/
static void
monteCarloRadiosityUpdateImportance(ELEMENT *elem) {
    if ( !monteCarloRadiosityForAllChildrenElements(elem, monteCarloRadiosityUpdateImportance)) {
        /* leaf element */
        float delta_imp = (PATCH_IS_VISIBLE(elem->pog.patch) ? 1. : 0.) - elem->source_imp;
        elem->imp += delta_imp;
        elem->source_imp += delta_imp;
        elem->unshot_imp += delta_imp;
        monteCarloRadiosityAccumulateImportances(elem);
    } else {
        /* not a leaf element: clear & pull importance */
        elem->imp = elem->source_imp = elem->unshot_imp = 0.;
        monteCarloRadiosityForAllChildrenElements(elem, monteCarloRadiosityPullImportances);
    }
}

/**
Re-init importance in the element hierarchy starting with the top cluster
*/
static void
monteCarloRadiosityReInitImportance(ELEMENT *elem) {
    if ( !monteCarloRadiosityForAllChildrenElements(elem, monteCarloRadiosityReInitImportance)) {
        // Leaf element
        elem->imp = elem->source_imp = elem->unshot_imp
                = PATCH_IS_VISIBLE(elem->pog.patch) ? 1.0 : 0.0;
        monteCarloRadiosityAccumulateImportances(elem);
    } else {
        // Not a leaf element: clear & pull importance
        elem->imp = elem->source_imp = elem->unshot_imp = 0.0;
        monteCarloRadiosityForAllChildrenElements(elem, monteCarloRadiosityPullImportances);
    }
}

void
monteCarloRadiosityUpdateViewImportance() {
    fprintf(stderr, "Updating direct visibility ... \n");

    UpdateDirectVisibility();

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp = 0.;
    monteCarloRadiosityUpdateImportance(GLOBAL_stochasticRaytracing_hierarchy.topcluster);

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp < GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp ) {
        fprintf(stderr, "Importance will be recomputed incrementally.\n");
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdatedFromScratch = false;
    } else {
        fprintf(stderr, "Importance will be recomputed from scratch.\n");
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdatedFromScratch = true;

        /* re-compute from scratch */
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp = 0.;
        monteCarloRadiosityReInitImportance(GLOBAL_stochasticRaytracing_hierarchy.topcluster);
    }

    GLOBAL_camera_mainCamera.changed = false;    /* indicate that direct importance has been
				 * computed for this view already. */
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays = 0;    /* start over */
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdated = true;
}

/**
Computes max_i (A_T/A_i): the ratio of the total area over the minimal patch
area in the scene, ignoring the 10% area occupied by the smallest patches
*/
static double
monteCarloRadiosityDetermineAreaFraction() {
    float *areas;
    float cumul;
    float areafrac;
    int nrpatchids = patchGetNextId();
    int i;

    if ( !GLOBAL_scene_world ) {
        return 100;
    }    /* an arbitrary positive number (in order
				 * to avoid divisions by zero */

    // Build a table of patch areas
    areas = (float *)malloc(nrpatchids * sizeof(float));
    for ( i = 0; i < nrpatchids; i++ ) {
        areas[i] = 0.;
    }
    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        areas[window->patch->id] = window->patch->area;
    }

    /* sort the table to decreasing areas */
    qsort((void *) areas,
          nrpatchids,
          sizeof(float),
          (QSORT_CALLBACK_TYPE)fcmp);

    // Find the patch such that 10% of the total surface area is filled by smaller patches
    for ( i = nrpatchids - 1, cumul = 0.; i >= 0 && cumul < GLOBAL_statistics_totalArea * 0.1; i-- ) {
        cumul += areas[i];
    }
    areafrac = (i >= 0 && areas[i] > 0.) ? GLOBAL_statistics_totalArea / areas[i] : GLOBAL_statistics_numberOfPatches;

    free(areas);

    return areafrac;
}

/**
Determines elementary ray power for the initial incremental iterations
*/
static void
monteCarloRadiosityDetermineInitialNrRays() {
    double areafrac = monteCarloRadiosityDetermineAreaFraction();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays = (long) ((double) GLOBAL_stochasticRaytracing_monteCarloRadiosityState.rayUnitsPerIt * areafrac);
}

/**
Really initialises: before the first iteration step
*/
void
monteCarloRadiosityReInit() {
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited ) {
        return;
    }

    fprintf(stderr, "Initialising Monte Carlo radiosity ...\n");

    SetSequence4D(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sequence);

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited = true;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds = 0.;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.lastClock = clock();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration = 0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevTracedRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.numberOfMisses = 0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevImportanceTracedRays = 0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.setSource = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedPaths = 0;
    colorClear(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance);

    colorClear(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp = 0.;
    colorClear(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp = 0.;
    colorClear(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux);
    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        Patch *patch = window->patch;
        monteCarloRadiosityInitPatch(patch);
        colorAddScaled(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux, M_PI * patch->area, getTopLevelPatchUnShotRad(patch)[0], GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux);
        colorAddScaled(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux, M_PI * patch->area, getTopLevelPatchRad(patch)[0], GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux);
        colorAddScaled(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux, M_PI * patch->area * (TOPLEVEL_ELEMENT(patch)->imp - TOPLEVEL_ELEMENT(patch)->source_imp), getTopLevelPatchUnShotRad(patch)[0],
                       GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp += patch->area * fabs(TOPLEVEL_ELEMENT(patch)->unshot_imp);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp += patch->area * TOPLEVEL_ELEMENT(patch)->imp;
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp += patch->area * TOPLEVEL_ELEMENT(patch)->source_imp;
        monteCarloRadiosityPatchComputeNewColor(patch);
    }

    monteCarloRadiosityDetermineInitialNrRays();

    elementHierarchyInit();

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
        monteCarloRadiosityUpdateViewImportance();
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdatedFromScratch = true;
    }
}

void
monteCarloRadiosityPreStep() {
    if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited ) {
        monteCarloRadiosityReInit();
    }
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven && GLOBAL_camera_mainCamera.changed ) {
        monteCarloRadiosityUpdateViewImportance();
    }

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.lastClock = clock();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration++;
}

/**
Undoes the effect of mainInit() and all side-effects of Step()
*/
void
monteCarloRadiosityTerminate() {
    elementHierarchyTerminate();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited = false;
}

static COLOR
monteCarloRadiosityDiffuseReflectanceAtPoint(Patch *patch, double u, double v) {
    RayHit hit;
    Vector3D point;
    patchUniformPoint(patch, u, v, &point);
    InitHit(&hit, patch, nullptr, &point, &patch->normal, patch->surface->material, 0.);
    hit.uv.u = u;
    hit.uv.v = v;
    hit.flags |= HIT_UV;
    return BsdfScatteredPower(hit.material->bsdf, &hit, &patch->normal, BRDF_DIFFUSE_COMPONENT);
}

static COLOR
monteCarloRadiosityInterpolatedReflectanceAtPoint(ELEMENT *leaf, double u, double v) {
    static ELEMENT *cachedleaf = nullptr;
    static COLOR vrd[4], rd;

    if ( leaf != cachedleaf ) {
        int i;
        for ( i = 0; i < leaf->nrvertices; i++ ) {
            vrd[i] = VertexReflectance(leaf->vertex[i]);
        }
    }
    cachedleaf = leaf;

    colorClear(rd);
    switch ( leaf->nrvertices ) {
        case 3:
            colorInterpolateBarycentric(vrd[0], vrd[1], vrd[2], u, v, rd);
            break;
        case 4:
            colorInterpolateBilinear(vrd[0], vrd[1], vrd[2], vrd[3], u, v, rd);
            break;
        default:
            logFatal(-1, "monteCarloRadiosityInterpolatedReflectanceAtPoint", "Invalid nr of vertices %d", leaf->nrvertices);
    }
    return rd;
}

/**
Returns the radiance emitted from the patch at the point with parameters
(u,v) into the direction 'dir'
*/
COLOR
monteCarloRadiosityGetRadiance(Patch *patch, double u, double v, Vector3D dir) {
    COLOR TrueRdAtPoint = monteCarloRadiosityDiffuseReflectanceAtPoint(patch, u, v);
    ELEMENT *leaf = monteCarloRadiosityRegularLeafElementAtPoint(TOPLEVEL_ELEMENT(patch), &u, &v);
    COLOR UsedRdAtPoint = renderopts.smooth_shading ? monteCarloRadiosityInterpolatedReflectanceAtPoint(leaf, u, v) : leaf->Rd;
    COLOR rad = ElementDisplayRadianceAtPoint(leaf, u, v);
    COLOR source_rad;
    colorClear(source_rad);

    /* subtract source radiance */
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show != SHOW_INDIRECT_RADIANCE ) {
        /* source_rad is self-emitted radiance if !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly. It is direct
         * illumination if GLOBAL_stochasticRaytracing_monteCarloRadiosityState.direct_only */
        if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.doNonDiffuseFirstShot ) {
            source_rad = leaf->source_rad;
        }
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly || GLOBAL_stochasticRaytracing_monteCarloRadiosityState.doNonDiffuseFirstShot ) {
            /* subtract self-emitted radiance */
            colorAdd(source_rad, leaf->Ed, source_rad);
        }
    }
    colorSubtract(rad, source_rad, rad);

    colorProduct(rad, TrueRdAtPoint, rad);
    colorDivide(rad, UsedRdAtPoint, rad);

    /* re-add source radiance */
    colorAdd(rad, source_rad, rad);

    return rad;
}

void
monteCarloRadiosityRecomputeDisplayColors() {
    if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited ) {
        return;
    }

    fprintf(stderr, "Recomputing display colors ...\n");
    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        monteCarloRadiosityPatchComputeNewColor(window->patch);
    }
}

void
monteCarloRadiosityUpdateMaterial(Material *oldMaterial, Material *newMaterial) {
    logError("monteCarloRadiosityUpdateMaterial", "Not yet implemented");
}

/**
Returns scalar reflectance, for importance propagation
*/
float
monteCarloRadiosityScalarReflectance(Patch *P) {
    return monteCarloRadiosityElementScalarReflectance(TOPLEVEL_ELEMENT(P));
}
