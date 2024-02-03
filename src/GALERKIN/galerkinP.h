/**
Galerkin Radiosity "private" declarations
*/

#ifndef __GALERKIN_P__
#define __GALERKIN_P__

#include "java/util/ArrayList.h"
#include "shared/cubature.h"
#include "SGL/sgl.h"
#include "GALERKIN/GalerkinElement.h"

/**
The Galerkin specific data for a patch is its toplevel element
*/

#define RADIANCE(patch) ((GalerkinElement *)(patch->radianceData))->radiance[0]
#define UN_SHOT_RADIANCE(patch) ((GalerkinElement *)(patch->radianceData))->unShotRadiance[0]
#define POTENTIAL(patch) ((GalerkinElement *)(patch->radianceData))->potential
#define UN_SHOT_POTENTIAL(patch) ((GalerkinElement *)(patch->radianceData))->unShotPotential
#define TOPLEVEL_ELEMENT(patch) ((GalerkinElement *)(patch->radianceData))

// Galerkin Radiosity "state" information
enum ITERATION_METHOD {
    JACOBI,
    GAUSS_SEIDEL,
    SOUTHWELL
};

enum SHAFTCULLMODE {
    ALWAYS_DO_SHAFTCULLING,
    DO_SHAFTCULLING_FOR_REFINEMENT
};

enum CUBATURE_DEGREE {
    DEGREE_1,
    DEGREE_2,
    DEGREE_3,
    DEGREE_4,
    DEGREE_5,
    DEGREE_6,
    DEGREE_7,
    DEGREE_8,
    DEGREE_9,
    DEGREE_3_PROD,
    DEGREE_5_PROD,
    DEGREE_7_PROD
};

enum ERROR_NORM {
    RADIANCE_ERROR,
    POWER_ERROR
};

enum BASIS_TYPE {
    CONSTANT,
    LINEAR,
    QUADRATIC,
    CUBIC
};

// Determines how source radiance of a source cluster is determined and
// how irradiance is distributed over the patches in a receiver cluster
enum CLUSTERING_STRATEGY {
    ISOTROPIC,
    ORIENTED,
    Z_VISIBILITY
};

class GALERKIN_STATE {
  public:
    int iteration_nr; // Number of iterations and nr of steps
    int hierarchical; // Set true for hierarchical refinement
    int importance_driven; // Set true for potential-driven comp
    int clustered; // Set true for clustering
    ITERATION_METHOD iteration_method; // How to solve the resulting linear set
    int lazy_linking; // Set true for lazy linking
    int exact_visibility; // For more exact treatment of visibility
    int multires_visibility; // For multi-resolution visibility determination
    int use_constant_radiance; // Set true for constant radiance initialization
    int use_ambient_radiance; // Ambient radiance (for visualisation only)
    COLOR constant_radiance;
    COLOR ambient_radiance;
    SHAFTCULLMODE shaftcullmode; // When to do shaft culling

    // Cubature rules for computing form factors
    CUBATURE_DEGREE rcv_degree;
    CUBATURE_DEGREE src_degree;
    CUBARULE *rcv3rule;
    CUBARULE *rcv4rule;
    CUBARULE *src3rule;
    CUBARULE *src4rule;
    CUBARULE *clusRule;

    // Global variables concerning clustering
    GalerkinElement *top_cluster; // Top level cluster containing the whole scene
    Geometry *top_geom; // A single COMPOUND Geometry containing the whole scene

    // Parameters that control accuracy
    ERROR_NORM error_norm; // Control radiance or power error?
    float rel_min_elem_area; /* subdivision of elements that are smaller
			    * than the total surface area of the scene
			    * times this number, will not be allowed. */
    float rel_link_error_threshold;  /* relative to max. selfemitted radiance
			    * when controlling the radiance error and
			    * to the max. selfemitted power when controlling
			    * the power error. */

    BASIS_TYPE basis_type; // Determines max. approximation order

    // Clustering strategy
    CLUSTERING_STRATEGY clustering_strategy;

    // Some global variables for form-factor computation
    GalerkinElement *fflastrcv;
    GalerkinElement *fflastsrc;

    // Scratch offscreen renderer for various clustering operations
    SGL_CONTEXT *scratch;
    int scratch_fb_size; // Scratch frame buffer size
    int lastclusid; // Used for caching cluster and eye point
    Vector3D lasteye; // Rendered into the scratch frame buffer

    unsigned long lastclock; // For CPU timing
    float cpu_secs;
};

extern GALERKIN_STATE GLOBAL_galerkin_state;

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

enum ROLE {
    SOURCE,
    RECEIVER
};

// In GalerkingRadiosity.cpp
extern void setCubatureRules(CUBARULE **triRule, CUBARULE **quadRule, CUBATURE_DEGREE degree);
extern void patchRecomputeColor(Patch *patch);

// In shooting.cpp
extern int doShootingStep(java::ArrayList<Patch *> *scenePatches);

// In gathering.cpp
extern int randomWalkRadiosityDoGatheringIteration(java::ArrayList<Patch *> *scenePatches);
extern int doClusteredGatheringIteration(java::ArrayList<Patch *> *scenePatches);

// In initiallinking.cpp
extern void createInitialLinks(GalerkinElement *top, ROLE role);
extern void createInitialLinkWithTopCluster(GalerkinElement *elem, ROLE role);

// In hierefine.cpp
extern void refineInteractions(GalerkinElement *top);

// In basisgalerkin.cpp
extern void basisGalerkinPushPullRadiance(GalerkinElement *top);

#endif
