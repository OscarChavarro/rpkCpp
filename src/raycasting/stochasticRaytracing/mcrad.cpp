/* Monte Carlo Radiosity: common code for stochastic relaxation and random walks */

#include <cstdlib>

#include "common/error.h"
#include "skin/patch_flags.h"
#include "material/statistics.h"
#include "scene/scene.h"
#include "skin/Vertex.h"
#include "shared/options.h"
#include "shared/potential.h"
#include "shared/camera.h"
#include "shared/render.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/elementmcrad.h"

STATE mcr;

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
        {RW_NLAST,        "Last-N",     2},
        {0, nullptr,                       0}
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
        {"-srr-ray-units", 8, Tint, &mcr.ray_units_per_it, DEFAULT_ACTION,
         "-srr-ray-units <n>          : To tune the amount of work in a single iteration"},
        {"-srr-bidirectional", 7, Tbool, &mcr.bidirectional_transfers, DEFAULT_ACTION,
         "-srr-bidirectional <yes|no> : Use lines bidirectionally"},
        {"-srr-control-variate", 7, Tbool, &mcr.constant_control_variate, DEFAULT_ACTION,
         "-srr-control-variate <y|n>  : Constant Control Variate variance reduction"},
        {"-srr-indirect-only", 7, Tbool, &mcr.indirect_only, DEFAULT_ACTION,
         "-srr-indirect-only <y|n>    : Compute indirect illumination only"},
        {"-srr-importance-driven", 7, Tbool, &mcr.importance_driven, DEFAULT_ACTION,
         "-srr-importance-driven <y|n>: Use view-importance"},
        {"-srr-sampling-sequence", 7, Tsequence, &mcr.sequence, DEFAULT_ACTION,
         "-srr-sampling-sequence <type>: \"PseudoRandom\", \"Niederreiter\""},
        {"-srr-approximation", 7, Tapprox, &mcr.approx_type, DEFAULT_ACTION,
         "-srr-approximation <order>  : \"constant\", \"linear\", \"quadratic\", \"cubic\""},
        {"-srr-hierarchical", 7, Tbool, &hierarchy.do_h_meshing, DEFAULT_ACTION,
         "-srr-hierarchical <y|n>     : hierarchical refinement"},
        {"-srr-clustering", 7, Tclustering, &hierarchy.clustering, DEFAULT_ACTION,
         "-srr-clustering <mode>      : \"none\", \"isotropic\", \"oriented\""},
        {"-srr-epsilon", 7, Tfloat, &hierarchy.epsilon, DEFAULT_ACTION,
         "-srr-epsilon <float>        : link power threshold (relative w.r.t. max. selfemitted power)"},
        {"-srr-minarea", 7, Tfloat, &hierarchy.minarea, DEFAULT_ACTION,
         "-srr-minarea <float>        : minimal element area (relative w.r.t. total area)"},
        {"-srr-display", 7, TshowWhat, &mcr.show, DEFAULT_ACTION,
         "-srr-display <what>         : \"total-radiance\", \"indirect-radiance\", \"weighting-gain\", \"importance\""},
        {"-srr-discard-incremental", 7, Tbool, &mcr.discard_incremental, DEFAULT_ACTION,
         "-srr-discard-incremenal <y|n>: Discard result of first iteration (incremental steps)"},
        {"-srr-incremental-uses-importance", 7, Tbool, &mcr.incremental_uses_importance, DEFAULT_ACTION,
         "-srr-incremental-uses-importance <y|n>: Use view-importance sampling already for the first iteration (incremental steps)"},
        {"-srr-naive-merging", 7, Tbool, &mcr.naive_merging, DEFAULT_ACTION,
         "-srr-naive-merging <y|n>    : disable intelligent merging heuristic"},
        {"-srr-nondiffuse-first-shot", 7, Tbool, &mcr.do_nondiffuse_first_shot, DEFAULT_ACTION,
         "-srr-nondiffuse-first-shot <y|n>: Do Non-diffuse first shot before real work"},
        {"-srr-initial-ls-samples", 7, Tint, &mcr.initial_ls_samples, DEFAULT_ACTION,
         "-srr-initial-ls-samples <int>        : nr of samples per light source for initial shot"},
        {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION,
         nullptr}
};

static CMDLINEOPTDESC rwrOptions[] = {
        {"-rwr-ray-units", 8, Tint, &mcr.ray_units_per_it, DEFAULT_ACTION,
         "-rwr-ray-units <n>          : To tune the amount of work in a single iteration"},
        {"-rwr-continuous", 7, Tbool, &mcr.continuous_random_walk, DEFAULT_ACTION,
         "-rwr-continuous <y|n>       : Continuous (yes) or Discrete (no) random walk"},
        {"-rwr-control-variate", 7, Tbool, &mcr.constant_control_variate, DEFAULT_ACTION,
         "-rwr-control-variate <y|n>  : Constant Control Variate variance reduction"},
        {"-rwr-indirect-only", 7, Tbool, &mcr.indirect_only, DEFAULT_ACTION,
         "-rwr-indirect-only <y|n>    : Compute indirect illumination only"},
        {"-rwr-sampling-sequence", 7, Tsequence, &mcr.sequence, DEFAULT_ACTION,
         "-rwr-sampling-sequence <type>: \"PseudoRandom\", \"Halton\", \"Niederreiter\""},
        {"-rwr-approximation", 7, Tapprox, &mcr.approx_type, DEFAULT_ACTION,
         "-rwr-approximation <order>  : \"constant\", \"linear\", \"quadratic\", \"cubic\""},
        {"-rwr-estimator", 7, TestType, &mcr.rw_estimator_type, DEFAULT_ACTION,
         "-rwr-estimator <type>       : \"shooting\", \"gathering\""},
        {"-rwr-score", 7, TestKind, &mcr.rw_estimator_kind, DEFAULT_ACTION,
         "-rwr-score <kind>           : \"collision\", \"absorption\", \"survival\", \"last-N\", \"last-but-N\""},
        {"-rwr-numlast", 12, Tint, &mcr.rw_numlast, DEFAULT_ACTION,
         "-rwr-numlast <int>          : N to use in \"last-N\" and \"last-but-N\" scorers"},
        {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION,
         nullptr}
};

void McrDefaults() {
    mcr.hack = false;
    mcr.inited = false;
    mcr.no_smoothing = false;
    mcr.ray_units_per_it = 10;
    mcr.bidirectional_transfers = false;
    mcr.constant_control_variate = false;
    colorClear(mcr.control_radiance);
    mcr.indirect_only = false;
    mcr.sequence = S4D_NIEDERREITER;
    mcr.approx_type = AT_CONSTANT;
    mcr.importance_driven = false;
    mcr.radiance_driven = true;
    mcr.importance_updated = false;
    mcr.importance_updated_from_scratch = false;
    mcr.continuous_random_walk = false;
    mcr.rw_estimator_type = RW_SHOOTING;
    mcr.rw_estimator_kind = RW_COLLISION;
    mcr.rw_numlast = 1;
    mcr.k_factor = 1.;
    mcr.show_shooting_weights = false;
    mcr.weighted_sampling = false;
    mcr.fake_global_lines = false;
    mcr.discard_incremental = false;
    mcr.incremental_uses_importance = false;
    mcr.naive_merging = false;

    mcr.show = SHOW_TOTAL_RADIANCE;
    mcr.show_weighted = SHOW_NON_WEIGHTED;

    mcr.do_nondiffuse_first_shot = false;
    mcr.initial_ls_samples = 1000;

    ElementHierarchyDefaults();
    InitBasis();
}

void SrrParseOptions(int *argc, char **argv) {
    ParseOptions(srrOptions, argc, argv);
}

void SrrPrintOptions(FILE *fp) {
}

void RwrParseOptions(int *argc, char **argv) {
    ParseOptions(rwrOptions, argc, argv);
}

void RwrPrintOptions(FILE *fp) {
}

/* for counting how much CPU time was used for the computations */
void McrUpdateCpuSecs() {
    clock_t t;

    t = clock();
    mcr.cpu_secs += (float) (t - mcr.lastclock) / (float) CLOCKS_PER_SEC;
    mcr.lastclock = t;
}

void *McrCreatePatchData(PATCH *patch) {
    patch->radiance_data = (void *) CreateToplevelSurfaceElement(patch);
    return patch->radiance_data;
}

void McrPrintPatchData(FILE *out, PATCH *patch) {
    PrintElement(out, TOPLEVEL_ELEMENT(patch));
}

void McrDestroyPatchData(PATCH *patch) {
    if ( patch->radiance_data ) {
        McrDestroyToplevelSurfaceElement(TOPLEVEL_ELEMENT(patch));
    }
    patch->radiance_data = (void *) nullptr;
}

/* compute new color for the patch: fine if no hierarchical refinement is used, e.g.
 * in the current random walk radiosity implementation */
void McrPatchComputeNewColor(PATCH *patch) {
    patch->color = ElementColor(TOPLEVEL_ELEMENT(patch));
    PatchComputeVertexColors(patch);
}

/* Initializes the computations for the current scene (if any): initialisations
 * are delayed to just before the first iteration step, see ReInit() below. */
void McrInit() {
    mcr.inited = false;
}

/* initialises patch data */
static void McrInitPatch(PATCH *P) {
    COLOR Ed = EMITTANCE(P);

    ReAllocCoefficients(TOPLEVEL_ELEMENT(P));
    CLEARCOEFFICIENTS(RAD(P), BAS(P));
    CLEARCOEFFICIENTS(UNSHOT_RAD(P), BAS(P));
    CLEARCOEFFICIENTS(RECEIVED_RAD(P), BAS(P));

    RAD(P)[0] = UNSHOT_RAD(P)[0] = SOURCE_RAD(P) = Ed;
    colorClear(RECEIVED_RAD(P)[0]);

    RAY_INDEX(P) = P->id * 11;
    QUALITY(P) = 0.;
    NG(P) = 0;

    IMP(P) = UNSHOT_IMP(P) = RECEIVED_IMP(P) = SOURCE_IMP(P) = 0.;
}

/* routines below update/re-initialise importance after a viewing change */
static void PullImportances(ELEMENT *child) {
    ELEMENT *parent = child->parent;
    PullImportance(parent, child, &parent->imp, &child->imp);
    PullImportance(parent, child, &parent->source_imp, &child->source_imp);
    PullImportance(parent, child, &parent->unshot_imp, &child->unshot_imp);
}

static void AccumulateImportances(ELEMENT *elem) {
    mcr.total_ymp += elem->area * elem->imp;
    mcr.source_ymp += elem->area * elem->source_imp;
    mcr.unshot_ymp += elem->area * fabs(elem->unshot_imp);
}

/* update importance in the element hierarchy starting with the top cluster */
static void UpdateImportance(ELEMENT *elem) {
    if ( !McrForAllChildrenElements(elem, UpdateImportance)) {
        /* leaf element */
        float delta_imp = (PATCH_IS_VISIBLE(elem->pog.patch) ? 1. : 0.) - elem->source_imp;
        elem->imp += delta_imp;
        elem->source_imp += delta_imp;
        elem->unshot_imp += delta_imp;
        AccumulateImportances(elem);
    } else {
        /* not a leaf element: clear & pull importance */
        elem->imp = elem->source_imp = elem->unshot_imp = 0.;
        McrForAllChildrenElements(elem, PullImportances);
    }
}

/* re-init importance in the element hierarchy starting with the top cluster */
static void ReInitImportance(ELEMENT *elem) {
    if ( !McrForAllChildrenElements(elem, ReInitImportance)) {
        /* leaf element */
        elem->imp = elem->source_imp = elem->unshot_imp
                = PATCH_IS_VISIBLE(elem->pog.patch) ? 1. : 0.;
        AccumulateImportances(elem);
    } else {
        /* not a leaf element: clear & pull importance */
        elem->imp = elem->source_imp = elem->unshot_imp = 0.;
        McrForAllChildrenElements(elem, PullImportances);
    }
}

void McrUpdateViewImportance() {
    fprintf(stderr, "Updating direct visibility ... \n");

    UpdateDirectVisibility();

    mcr.source_ymp = mcr.unshot_ymp = mcr.total_ymp = 0.;
    UpdateImportance(hierarchy.topcluster);

    if ( mcr.unshot_ymp < mcr.source_ymp ) {
        fprintf(stderr, "Importance will be recomputed incrementally.\n");
        mcr.importance_updated_from_scratch = false;
    } else {
        fprintf(stderr, "Importance will be recomputed from scratch.\n");
        mcr.importance_updated_from_scratch = true;

        /* re-compute from scratch */
        mcr.source_ymp = mcr.unshot_ymp = mcr.total_ymp = 0.;
        ReInitImportance(hierarchy.topcluster);
    }

    GLOBAL_camera_mainCamera.changed = false;    /* indicate that direct importance has been
				 * computed for this view already. */
    mcr.imp_traced_rays = 0;    /* start over */
    mcr.importance_updated = true;
}

/* computes max_i (A_T/A_i): the ratio of the total area over the minimal patch 
 * area in the scene, ignoring the 10% area occupied by the smallest patches. */
static double DetermineAreaFraction() {
    float *areas, cumul, areafrac;
    int nrpatchids = PatchGetNextID(), i;

    if ( !GLOBAL_scene_world ) {
        return 100;
    }    /* an arbitrary positive number (in order
				 * to avoid divisions by zero */

    /* build a table of patch areas */
    areas = (float *)malloc(nrpatchids * sizeof(float));
    for ( i = 0; i < nrpatchids; i++ ) {
        areas[i] = 0.;
    }
    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    areas[P->id] = P->area;
                }
    EndForAll;

    /* sort the table to decreasing areas */
    qsort((void *) areas,
          nrpatchids,
          sizeof(float),
          (QSORT_CALLBACK_TYPE)fcmp);

    /* find the patch such that 10% of the total surface area is filled by
     * smaller patches. */
    for ( i = nrpatchids - 1, cumul = 0.; i >= 0 && cumul < GLOBAL_statistics_totalArea * 0.1; i-- ) {
        cumul += areas[i];
    }
    areafrac = (i >= 0 && areas[i] > 0.) ? GLOBAL_statistics_totalArea / areas[i] : GLOBAL_statistics_numberOfPatches;

    free((char *) areas);

    return areafrac;
}

/* determines elementary ray power for the initial incremental iterations. */
static void McrDetermineInitialNrRays() {
    double areafrac = DetermineAreaFraction();
    mcr.initial_nr_rays = (long) ((double) mcr.ray_units_per_it * areafrac);
}

/* really initialises: before the first iteration step */
void McrReInit() {
    if ( mcr.inited ) {
        return;
    }

    fprintf(stderr, "Initialising Monte Carlo radiosity ...\n");

    SetSequence4D(mcr.sequence);

    mcr.inited = true;
    mcr.cpu_secs = 0.;
    mcr.lastclock = clock();
    mcr.iteration_nr = 0;
    mcr.traced_rays = mcr.prev_traced_rays = mcr.nrmisses = 0;
    mcr.imp_traced_rays = mcr.prev_imp_traced_rays = 0;
    mcr.set_source = mcr.indirect_only;
    mcr.traced_paths = 0;
    colorClear(mcr.control_radiance);
    mcr.nr_weighted_rays = mcr.old_nr_weighted_rays = 0;

    colorClear(mcr.unshot_flux);
    mcr.unshot_ymp = 0.;
    colorClear(mcr.total_flux);
    mcr.total_ymp = 0.;
    colorClear(mcr.imp_unshot_flux);
    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    McrInitPatch(P);
                    colorAddScaled(mcr.unshot_flux, M_PI * P->area, UNSHOT_RAD(P)[0], mcr.unshot_flux);
                    colorAddScaled(mcr.total_flux, M_PI * P->area, RAD(P)[0], mcr.total_flux);
                    colorAddScaled(mcr.imp_unshot_flux, M_PI * P->area * (IMP(P) - SOURCE_IMP(P)), UNSHOT_RAD(P)[0],
                                   mcr.imp_unshot_flux);
                    mcr.unshot_ymp += P->area * fabs(UNSHOT_IMP(P));
                    mcr.total_ymp += P->area * IMP(P);
                    mcr.source_ymp += P->area * SOURCE_IMP(P);
                    McrPatchComputeNewColor(P);
                }
    EndForAll;

    McrDetermineInitialNrRays();

    ElementHierarchyInit();

    if ( mcr.importance_driven ) {
        McrUpdateViewImportance();
        mcr.importance_updated_from_scratch = true;
    }
}

void McrPreStep() {
    if ( !mcr.inited ) {
        McrReInit();
    }
    if ( mcr.importance_driven && GLOBAL_camera_mainCamera.changed ) {
        McrUpdateViewImportance();
    }

    mcr.wake_up = false;
    mcr.lastclock = clock();

    mcr.iteration_nr++;
}

/* undoes the effect of Init() and all side-effects of Step() */
void McrTerminate() {
    ElementHierarchyTerminate();
    mcr.inited = false;
}

static COLOR McrDiffuseReflectanceAtPoint(PATCH *patch, double u, double v) {
    HITREC hit;
    Vector3D point;
    PatchUniformPoint(patch, u, v, &point);
    InitHit(&hit, patch, nullptr, &point, &patch->normal, patch->surface->material, 0.);
    hit.uv.u = u;
    hit.uv.v = v;
    hit.flags |= HIT_UV;
    return BsdfScatteredPower(hit.material->bsdf, &hit, &patch->normal, BRDF_DIFFUSE_COMPONENT);
}

static COLOR McrInterpolatedReflectanceAtPoint(ELEMENT *leaf, double u, double v) {
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
            Fatal(-1, "McrInterpolatedReflectanceAtPoint", "Invalid nr of vertices %d", leaf->nrvertices);
    }
    return rd;
}

/* Returns the radiance emitted from the patch at the point with parameters
 * (u,v) into the direction 'dir'. */
COLOR McrGetRadiance(PATCH *patch, double u, double v, Vector3D dir) {
    COLOR TrueRdAtPoint = McrDiffuseReflectanceAtPoint(patch, u, v);
    ELEMENT *leaf = RegularLeafElementAtPoint(TOPLEVEL_ELEMENT(patch), &u, &v);
    COLOR UsedRdAtPoint = renderopts.smooth_shading ? McrInterpolatedReflectanceAtPoint(leaf, u, v) : leaf->Rd;
    COLOR rad = ElementDisplayRadianceAtPoint(leaf, u, v);
    COLOR source_rad;
    colorClear(source_rad);

    /* subtract source radiance */
    if ( mcr.show != SHOW_INDIRECT_RADIANCE ) {
        /* source_rad is self-emitted radiance if !mcr.indirect_only. It is direct
         * illumination if mcr.direct_only */
        if ( !mcr.do_nondiffuse_first_shot ) {
            source_rad = leaf->source_rad;
        }
        if ( mcr.indirect_only || mcr.do_nondiffuse_first_shot ) {
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

void McrRecomputeDisplayColors() {
    if ( !mcr.inited ) {
        return;
    }

    fprintf(stderr, "Recomputing display colors ...\n");
    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    McrPatchComputeNewColor(P);
                }
    EndForAll;
}

void McrUpdateMaterial(MATERIAL *oldmaterial, MATERIAL *newmaterial) {
    Error("McrUpdateMaterial", "Not yet implemented");
}

/* for (scalar) importance propagation */
float McrScalarReflectance(PATCH *P) {
    return ElementScalarReflectance(TOPLEVEL_ELEMENT(P));
}

/* sample based variance estimate */
double VarianceEstimate(double N, double sum_of_squares, double square_of_sum) {
    return 1. / (N - 1.) * (sum_of_squares / N - square_of_sum / (N * N));
}

void RwrCreateControlPanel(void *parent_widget) {}

void RwrUpdateControlPanel(void *parent_widget) {}

void RwrShowControlPanel() {}

void RwrHideControlPanel() {}

void SrrCreateControlPanel(void *parent_widget) {}

void SrrUpdateControlPanel(void *parent_widget) {}

void SrrShowControlPanel() {}

void SrrHideControlPanel() {}
