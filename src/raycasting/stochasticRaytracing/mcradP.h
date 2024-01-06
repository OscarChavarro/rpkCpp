#ifndef _MCRADP_H_
#define _MCRADP_H_

#include <ctime>

#include "raycasting/stochasticRaytracing/sample4d.h"
#include "raycasting/stochasticRaytracing/elementmcrad.h"
#include "raycasting/stochasticRaytracing/coefficientsmcrad.h"

/**
Global variables: same set for stochastic relaxation and for random walk
radiosity
*/

enum METHOD {
    STOCHASTIC_RELAXATION_RADIOSITY_METHOD,
    RANDOM_WALK_RADIOSITY_METHOD
};

enum RW_ESTIMATOR_TYPE {
    RW_SHOOTING,
    RW_GATHERING
};

enum RW_ESTIMATOR_KIND {
    RW_COLLISION,
    RW_ABSORPTION,
    RW_SURVIVAL,
    RW_LAST_BUT_NTH,
    RW_NLAST
};

enum SHOW_WHAT {
    SHOW_TOTAL_RADIANCE,
    SHOW_INDIRECT_RADIANCE,
    SHOW_IMPORTANCE
};

enum SHOW_WEIGHTING {
    SHOW_NON_WEIGHTED
};


class STATE {
  public:
    METHOD method;    /* stochastic relaxation or random walks */
    SHOW_WHAT show;    /* what to show and how to display the result */

    int inited;        /* flag indicating whether initialised or not */

    int iteration_nr;    /* current iteration number */
    COLOR unshot_flux;
    COLOR total_flux;
    COLOR imp_unshot_flux;    /* indirect-importance weighted unshot flux */
    float unshot_ymp, total_ymp, source_ymp;  /* sum over all patches of area * importance */

    int ray_units_per_it;    /* to increase or decrease initial nr of rays */
    int bidirectional_transfers;  /* for bidirectional energy transfers */
    int constant_control_variate;    /* for constant control variate variance reduction */
    COLOR control_radiance;    /* constant control radiance value */
    int indirect_only;        /* whether or not to compute indirect illum. only */

    int weighted_sampling;    /* whether or not to do weighted sampling ala
				 * Powell and Swann/Spanier */

    int set_source;        /* for copying direct illum. to SOURCE_RAD(..)
				 * if computing only indirect illum. */
    SEQ4D sequence;        /* random number sequence, see sample4d.h */
    APPROX_TYPE approx_type;    /* radiosity approximation order */
    int importance_driven;    /* whether or not to use view-importance */
    int radiance_driven;        /* radiance-driven importance propagation */
    int importance_updated;    /* direct importance got updated or not? */
    int importance_updated_from_scratch;    /* can either incremental or from scratch */
    int continuous_random_walk;    /* continuous or discrete random walk */
    RW_ESTIMATOR_TYPE rw_estimator_type;    /* shooting, gathering, gathering for free */
    RW_ESTIMATOR_KIND rw_estimator_kind;    /* collision, absorption, ... */
    int rw_numlast;        /* for last-but-n and n-last RW estimators */
    float k_factor;        /* Mateu's mysterious gathering for free k-factor */
    int show_shooting_weights;    /* display shooting weights (gathering for free) */
    int discard_incremental;    /* first iteration (incremental steps) results are
				 * discarded in later computations */
    int incremental_uses_importance; /* view-importance is used already for the first
				 * iteration (incremental steps). This may confuse the
				 * merging heuristic or other things .... */
    int naive_merging;        /* results of different iterations are merged
				 * solely based on the number of rays shot */

    long initial_nr_rays;        /* for first iteration step */
    long rays_per_iteration;    /* for later iterations */
    long imp_rays_per_iteration;    /* for later iterations for importance */
    long traced_rays, prev_traced_rays;    /* total nr of traced rays and previous total */
    long imp_traced_rays, prev_imp_traced_rays;
    long traced_paths;        /* nr of traced random walks in random walk rad. */
    long nrmisses;        /* rays disappearing to background */
    int do_nondiffuse_first_shot; /* initial shooting pass handles non-diffuse lights */
    int initial_ls_samples;      /* initial shot samples per light source */

    int wake_up;        /* to react on GUI input now and then. */
    clock_t lastclock;    /* for computation timings */
    float cpu_secs;    /* CPU time spent in calculations */
};

extern STATE mcr;

inline int
NR_VERTICES(ELEMENT *elem) {
    return elem->pog.patch->nrvertices;
}

inline ELEMENT*
TOPLEVEL_ELEMENT(PATCH *patch) {
    return (ELEMENT *)patch->radiance_data;
}

inline COLOR *
getTopLevelPatchRad(PATCH *patch) {
    return TOPLEVEL_ELEMENT(patch)->rad;
}

inline COLOR *
getTopLevelPatchUnShotRad(PATCH *patch) {
    return TOPLEVEL_ELEMENT(patch)->unshot_rad;
}

inline COLOR*
getTopLevelPatchReceivedRad(PATCH *patch) {
    return TOPLEVEL_ELEMENT(patch)->received_rad;
}

inline GalerkinBasis *
getTopLevelPatchBasis(PATCH *patch) {
    return TOPLEVEL_ELEMENT(patch)->basis;
}

extern float monteCarloRadiosityScalarReflectance(PATCH *P);
extern void monteCarloRadiosityDefaults();
extern void monteCarloRadiosityUpdateCpuSecs();
extern void *monteCarloRadiosityCreatePatchData(PATCH *patch);
extern void monteCarloRadiosityPrintPatchData(FILE *out, PATCH *patch);
extern void monteCarloRadiosityDestroyPatchData(PATCH *patch);
extern void monteCarloRadiosityPatchComputeNewColor(PATCH *patch);
extern void monteCarloRadiosityInit();
extern void monteCarloRadiosityUpdateViewImportance();
extern void monteCarloRadiosityReInit();
extern void monteCarloRadiosityPreStep();
extern void monteCarloRadiosityTerminate();
extern COLOR monteCarloRadiosityGetRadiance(PATCH *patch, double u, double v, Vector3D dir);
extern void monteCarloRadiosityRecomputeDisplayColors();
extern void monteCarloRadiosityUpdateMaterial(MATERIAL *oldMaterial, MATERIAL *newMaterial);

extern void stochasticRelaxationRadiosityParseOptions(int *argc, char **argv);
extern void stochasticRelaxationRadiosityPrintOptions(FILE *fp);
extern void stochasticRelaxationRadiosityCreateControlPanel(void *parent_widget);
extern void stochasticRelaxationRadiosityUpdateControlPanel(void *parent_widget);
extern void stochasticRelaxationRadiosityShowControlPanel();
extern void stochasticRelaxationRadiosityHideControlPanel();

extern void randomWalkRadiosityParseOptions(int *argc, char **argv);
extern void randomWalkRadiosityPrintOptions(FILE *fp);
extern void randomWalkRadiosityCreateControlPanel(void *parent_widget);
extern void randomWalkRadiosityUpdateControlPanel(void *parent_widget);
extern void randomWalkRadiosityShowControlPanel();
extern void randomWalkRadiosityHideControlPanel();

extern void doNonDiffuseFirstShot();

#endif
