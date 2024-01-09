/* galerkinP.h: Galerkin Radisoity "private" declarations */

#ifndef _GALERKINP_H_
#define _GALERKINP_H_

#include <cstdio>

#include "common/mymath.h"
#include "material/color.h"
#include "shared/cubature.h"
#include "SGL/sgl.h"
#include "GALERKIN/elementgalerkin.h"

/* the Galerkin specifix data for a patch is its toplevel element. The
 * ELEMENT structure is defined in element.h. */

/* to save typing work: */
#define RADIANCE(patch) ((ELEMENT *)(patch->radiance_data))->radiance[0]
#define UNSHOT_RADIANCE(patch) ((ELEMENT *)(patch->radiance_data))->unshot_radiance[0]
#define POTENTIAL(patch) ((ELEMENT *)(patch->radiance_data))->potential
#define UNSHOT_POTENTIAL(patch) ((ELEMENT *)(patch->radiance_data))->unshot_potential
#define SELFEMITTED_RADIANCE(patch) ((ELEMENT *)(patch->radiance_data))->Ed
#define REFLECTIVITY(patch) ((ELEMENT *)(patch->radiance_data))->Rd
#define TOPLEVEL_ELEMENT(patch) ((ELEMENT *)(patch->radiance_data))

/* Galerkin Radiosity "state" information */
enum ITERATION_METHOD {
    JACOBI, GAUSS_SEIDEL, SOUTHWELL
};

enum SHAFTCULLMODE {
    ALWAYS_DO_SHAFTCULLING, NEVER_DO_SHAFTCULLING, DO_SHAFTCULLING_FOR_REFINEMENT
};

enum CUBATURE_DEGREE {
    DEGREE_1, DEGREE_2, DEGREE_3, DEGREE_4, DEGREE_5, DEGREE_6, DEGREE_7,
    DEGREE_8, DEGREE_9, DEGREE_3_PROD, DEGREE_5_PROD, DEGREE_7_PROD
};

enum ERROR_NORM {
    RADIANCE_ERROR, POWER_ERROR
};

enum BASIS_TYPE {
    CONSTANT, LINEAR, QUADRATIC, CUBIC
};

/* determines how source radiance of a source cluster is determined and
 * how irradiance is distributed over the patches in a receiver cluster */
enum CLUSTERING_STRATEGY {
    ISOTROPIC, ORIENTED, Z_VISIBILITY
};

class GALERKIN_STATE {
  public:
    int iteration_nr;    /* nr of iterations and nr of steps */
    int hierarchical;        /* set true for hierarchical refinement */
    int importance_driven;    /* set true for potential-driven comp. */
    int clustered;        /* set true for clustering */
    ITERATION_METHOD iteration_method; /* how to solve the resulting linear set */
    int lazy_linking;        /* set true for lazy linking */
    int exact_visibility;        /* for more exact treatment of visibility */
    int multires_visibility;    /* for multiresolution visibility determination */
    int use_constant_radiance;    /* set true for constant radiance initialization */
    int use_ambient_radiance;    /* ambient radiance (for visualisation only) */
    COLOR constant_radiance;
    COLOR ambient_radiance;
    SHAFTCULLMODE shaftcullmode;    /* when to do shaftculling */

    /* cubature rules for computing form factors */
    CUBATURE_DEGREE rcv_degree;
    CUBATURE_DEGREE src_degree;
    CUBARULE *rcv3rule;
    CUBARULE *rcv4rule;
    CUBARULE *src3rule;
    CUBARULE *src4rule;
    CUBARULE *clusRule;

    /* global variables concerning clustering */
    ELEMENT *top_cluster;    /* toplevel cluster containing the whole scene */
    Geometry *top_geom;    /* a single COMPOUND Geometry containing the whole scene */

    /* parameters that control accuracy */
    ERROR_NORM error_norm;   /* control radiance or power error? */
    float rel_min_elem_area; /* subdivision of elements that are smaller
			    * than the total surface area of the scene
			    * times this number, will not be allowed. */
    float rel_link_error_threshold;  /* relative to max. selfemitted radiance
			    * when controlling the radiance error and
			    * to the max. selfemitted power when controlling
			    * the power error. */

    BASIS_TYPE basis_type;    /* determines max. approximation order */

    /* clustering strategy */
    CLUSTERING_STRATEGY clustering_strategy;

    /* some global variables for formfactor computation */
    ELEMENT *fflastrcv;
    ELEMENT *fflastsrc;

    /* scratch offscreen renderer for various clustering operations. */
    SGL_CONTEXT *scratch;
    int scratch_fb_size;        /* scratch frame buffer size */
    int lastclusid;        /* used for caching cluster and eyepoint */
    Vector3D lasteye;        /* rendered into the scratch frame buffer */

    long lastclock;        /* for CPU timing */
    float cpu_secs;
    int wake_up;            /* for waking up now and then */
};

extern GALERKIN_STATE GLOBAL_galerkin_state;

/* default settings */
#define DEFAULT_GAL_HIERARCHICAL true
#define DEFAULT_GAL_IMPORTANCE_DRIVEN false
#define DEFAULT_GAL_CLUSTERED true
#define DEFAULT_GAL_ITERATION_METHOD JACOBI
#define DEFAULT_GAL_LAZY_LINKING true
#define DEFAULT_GAL_CONSTANT_RADIANCE false
#define DEFAULT_GAL_AMBIENT_RADIANCE false
#define DEFAULT_GAL_SHAFTCULLMODE DO_SHAFTCULLING_FOR_REFINEMENT
#define DEFAULT_GAL_RCV_CUBATURE_DEGREE DEGREE_5
#define DEFAULT_GAL_SRC_CUBATURE_DEGREE DEGREE_4
#define DEFAULT_GAL_REL_MIN_ELEM_AREA 1e-6
#define DEFAULT_GAL_REL_LINK_ERROR_THRESHOLD 1e-5
#define DEFAULT_GAL_ERROR_NORM POWER_ERROR
#define DEFAULT_GAL_BASIS_TYPE LINEAR
#define DEFAULT_GAL_EXACT_VISIBILITY true
#define DEFAULT_GAL_MULTIRES_VISIBILITY false
#define DEFAULT_GAL_CLUSTERING_STRATEGY ISOTROPIC
#define DEFAULT_GAL_SCRATCH_FB_SIZE 200

extern void initGalerkin();

/* installs cubature rules for triangles and quadrilaterals of the specified degree */
extern void setCubatureRules(CUBARULE **trirule, CUBARULE **quadrule, CUBATURE_DEGREE degree);

/* recomputes the color of a patch using ambient radiance term, ... if requested for */
extern void patchRecomputeColor(PATCH *patch);

/* in gathering.c. Returns true when converged and false if not. */
extern int randomWalkRadiosityDoGatheringIteration();

extern int doClusteredGatheringIteration();

extern void gatheringUpdateDirectPotential(ELEMENT *elem, float potential_increment);

extern void gatheringUpdateMaterial(Material *oldMaterial, Material *newMaterial);

/* in shooting.c. Returns true when converged and false if not. */
extern int doShootingStep();

extern void shootingUpdateDirectPotential(ELEMENT *elem, float potential_increment);

extern void shootingUpdateMaterial(Material *oldMaterial, Material *newMaterial);

enum ROLE {
    SOURCE,
    RECEIVER
};

/* Creates the initial interactions for a toplevel element which is
 * considered to be a SOURCE or RECEIVER according to 'role'. Interactions
 * are stored at the receiver element when doing gathering and at the
 * source element when doing shooting. */
extern void createInitialLinks(ELEMENT *top, ROLE role);

/* Creates an initial link between the given element and the top cluster. */
extern void createInitialLinkWithTopCluster(ELEMENT *elem, ROLE role);

/* recursively refines the interactions of the given toplevel element */
extern void RefineInteractions(ELEMENT *toplevelelement);

/* in basis.c: converts the received radiance of a patch into exitant
 * radiance, making a consistent hierarchical representation. */
extern void basisGalerkinPushPullRadiance(ELEMENT *top);

#endif
