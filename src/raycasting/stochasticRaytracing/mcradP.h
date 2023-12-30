/* mcradP.h */

#ifndef _MCRADP_H_
#define _MCRADP_H_

#include <ctime>

#include "material/color.h"
#include "raycasting/stochasticRaytracing/mcrad.h"
#include "raycasting/stochasticRaytracing/sample4d.h"
#include "raycasting/stochasticRaytracing/basismcrad.h"
#include "raycasting/stochasticRaytracing/elementmcrad.h"
#include "raycasting/stochasticRaytracing/coefficientsmcrad.h"

/* global variables: same set for stochastic relaxation and for random walk 
 * radiosity */

enum METHOD {
    SRR, RWR
};

enum RW_ESTIMATOR_TYPE {
    RW_SHOOTING, RW_GATHERING, RW_GFFMIS, RW_GFFVAR, RW_GFFHEUR
};

enum RW_ESTIMATOR_KIND {
    RW_COLLISION, RW_ABSORPTION, RW_SURVIVAL, RW_LAST_BUT_NTH, RW_NLAST
};

enum SHOW_WHAT {
    SHOW_TOTAL_RADIANCE, SHOW_INDIRECT_RADIANCE, SHOW_WEIGHTING_GAIN, SHOW_OLD_WEIGHTING_GAIN, SHOW_IMPORTANCE
};

enum SHOW_WEIGHTING {
    SHOW_NON_WEIGHTED, SHOW_WEIGHTED, SHOW_AUTO_WEIGHTED
};

class STATE {
  public:
    int hack;        /* for testing, should be FALSE normally */
    METHOD method;    /* stochastic relaxation or random walks */
    SHOW_WHAT show;    /* what to show and how to display the result */
    SHOW_WEIGHTING show_weighted;    /* with weighted sampling */

    int inited;        /* flag indicating whether initialised or not */
    int no_smoothing;    /* inhibits Gouraud interpolation in GetRadiance() */

    int iteration_nr;    /* current iteration number */
    COLOR unshot_flux, total_flux,
            imp_unshot_flux;    /* indirect-importance weighted unshot flux */
    float unshot_ymp, total_ymp, source_ymp;  /* sum over all patches of area * importance */

    int ray_units_per_it;    /* to increase or decrease initial nr of rays */
    int bidirectional_transfers;  /* for bidirectional energy transfers */
    int constant_control_variate;    /* for constant control variate variance reduction */
    COLOR control_radiance;    /* constant control radiance value */
    int indirect_only;        /* whether or not to compute indirect illum. only */

    int fake_global_lines;    /* imitates global line sampling */
    int weighted_sampling;    /* whether or not to do weighted sampling ala
				 * Powell and Swann/Spanier */
    long nr_weighted_rays, old_nr_weighted_rays;

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

#define NR_VERTICES(elem)       (elem->pog.patch->nrvertices)
#define TOPLEVEL_ELEMENT(patch)   ((ELEMENT *)(patch->radiance_data))

#define RAD(patch)        (TOPLEVEL_ELEMENT(patch)->rad)
#define UNSHOT_RAD(patch)    (TOPLEVEL_ELEMENT(patch)->unshot_rad)
#define RECEIVED_RAD(patch)    (TOPLEVEL_ELEMENT(patch)->received_rad)

#define BAS(patch)    (TOPLEVEL_ELEMENT(patch)->basis)
#define PRINTRAD(out, rad, patch)    PRINTCOEFFICIENTS(out, rad, patch)

#define SOURCE_RAD(patch)    (TOPLEVEL_ELEMENT(patch)->source_rad)
#define RAY_INDEX(patch)    (TOPLEVEL_ELEMENT(patch)->ray_index)
#define QUALITY(patch)        (TOPLEVEL_ELEMENT(patch)->quality)
#define NG(patch)        (TOPLEVEL_ELEMENT(patch)->ng)

#define IMP(patch)        (TOPLEVEL_ELEMENT(patch)->imp)
#define UNSHOT_IMP(patch)    (TOPLEVEL_ELEMENT(patch)->unshot_imp)
#define RECEIVED_IMP(patch)    (TOPLEVEL_ELEMENT(patch)->received_imp)
#define SOURCE_IMP(patch)    (TOPLEVEL_ELEMENT(patch)->source_imp)

#define REFLECTANCE(patch)    (TOPLEVEL_ELEMENT(patch)->Rd)
#define EMITTANCE(patch)    (TOPLEVEL_ELEMENT(patch)->Ed)

/* returns scalar reflectance, for importance propagation */
extern float McrScalarReflectance(PATCH *);

/* common routines for stochastic relaxation and random walks */
extern void McrDefaults();

extern void SrrParseOptions(int *argc, char **argv);

extern void SrrPrintOptions(FILE *fp);

extern void RwrParseOptions(int *argc, char **argv);

extern void RwrPrintOptions(FILE *fp);

extern void McrWakeUp(int sig);

extern void McrUpdateCpuSecs();

extern void *McrCreatePatchData(PATCH *patch);

extern void McrPrintPatchData(FILE *out, PATCH *patch);

extern void McrDestroyPatchData(PATCH *patch);

extern void McrPatchComputeNewColor(PATCH *patch);

extern void McrInit();

extern void McrUpdateViewImportance();

extern void McrReInit();

extern void McrPreStep();

extern void McrTerminate();

extern COLOR McrGetRadiance(PATCH *patch, double u, double v, Vector3D dir);

extern void McrRecomputeDisplayColors();

extern void McrUpdateMaterial(MATERIAL *oldmaterial, MATERIAL *newmaterial);

/* stochastic relaxation specific routines */
extern void SrrCreateControlPanel(void *parent_widget);

extern void SrrUpdateControlPanel(void *parent_widget);

extern void SrrShowControlPanel();

extern void SrrHideControlPanel();

/* random walk rad. specific routines */
extern void RwrCreateControlPanel(void *parent_widget);

extern void RwrUpdateControlPanel(void *parent_widget);

extern void RwrShowControlPanel();

extern void RwrHideControlPanel();

/* sample based variance estimate */
extern double VarianceEstimate(double N, double sum_of_squares, double square_of_sum);

/* initial shooting pass handling non-diffuse light sources */
extern void DoNonDiffuseFirstShot();

#endif
