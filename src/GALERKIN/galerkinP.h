/**
Galerkin Radiosity "private" declarations
*/

#ifndef __GALERKIN_P__
#define __GALERKIN_P__

#include "java/util/ArrayList.h"
#include "common/cubature.h"
#include "SGL/sgl.h"
#include "GALERKIN/GalerkinElement.h"

/**
The Galerkin specific data for a patch is its toplevel element
*/

#define RADIANCE(patch) ((GalerkinElement *)((patch)->radianceData))->radiance[0]
#define UN_SHOT_RADIANCE(patch) ((GalerkinElement *)((patch)->radianceData))->unShotRadiance[0]
#define POTENTIAL(patch) ((GalerkinElement *)((patch)->radianceData))->potential
#define UN_SHOT_POTENTIAL(patch) ((GalerkinElement *)((patch)->radianceData))->unShotPotential

inline GalerkinElement*
topLevelGalerkinElement(Patch *patch) {
    if ( patch == nullptr ) {
        fprintf(stderr, "Fatal: Trying to access as GalerkinElement on a null Patch\n");
        exit(1);
    }
    if ( patch->radianceData == nullptr ) {
        fprintf(stderr, "Fatal: Trying to access as GalerkinElement on a Patch with null radianceData\n");
        exit(1);
    }
    if ( patch->radianceData->className != ElementTypes::ELEMENT_GALERKIN ) {
        fprintf(stderr, "Fatal: Trying to access as GalerkinElement a different type of element\n");
        exit(1);
    }
    return (GalerkinElement *)patch->radianceData;
}

// Galerkin Radiosity "state" information
enum GalerkinIterationMethod {
    JACOBI,
    GAUSS_SEIDEL,
    SOUTH_WELL
};

enum GalerkinShaftCullMode {
    ALWAYS_DO_SHAFT_CULLING,
    DO_SHAFT_CULLING_FOR_REFINEMENT
};

enum GalerkinCubatureDegree {
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

enum GalerkinErrorNorm {
    RADIANCE_ERROR,
    POWER_ERROR
};

enum GalerkinBasisType {
    CONSTANT,
    LINEAR,
    QUADRATIC,
    CUBIC
};

// Determines how source radiance of a source cluster is determined and
// how irradiance is distributed over the patches in a receiver cluster
enum GalerkinClusteringStrategy {
    ISOTROPIC,
    ORIENTED,
    Z_VISIBILITY
};

class GalerkinState {
  public:
    int iteration_nr; // Number of iterations and nr of steps
    int hierarchical; // Set true for hierarchical refinement
    int importance_driven; // Set true for potential-driven comp
    int clustered; // Set true for clustering
    GalerkinIterationMethod iteration_method; // How to solve the resulting linear set
    int lazy_linking; // Set true for lazy linking
    int exact_visibility; // For more exact treatment of visibility
    int multiResolutionVisibility; // For multi-resolution visibility determination
    int use_constant_radiance; // Set true for constant radiance initialization
    int use_ambient_radiance; // Ambient radiance (for visualisation only)
    COLOR constant_radiance;
    COLOR ambient_radiance;
    GalerkinShaftCullMode shaftCullMode; // When to do shaft culling

    // Cubature rules for computing form factors
    GalerkinCubatureDegree rcvDegree;
    GalerkinCubatureDegree srcDegree;
    CUBARULE *rcv3rule;
    CUBARULE *rcv4rule;
    CUBARULE *src3rule;
    CUBARULE *src4rule;
    CUBARULE *clusterRule;

    // Global variables concerning clustering
    GalerkinElement *topCluster; // Top level cluster containing the whole scene
    Geometry *topGeometry; // A single COMPOUND Geometry containing the whole scene

    // Parameters that control accuracy
    GalerkinErrorNorm errorNorm; // Control radiance or power error?
    float relMinElemArea; /* subdivision of elements that are smaller
			    * than the total surface area of the scene
			    * times this number, will not be allowed. */
    float relLinkErrorThreshold;  /* relative to max. self-emitted radiance
			    * when controlling the radiance error and
			    * to the max. self-emitted power when controlling
			    * the power error. */

    GalerkinBasisType basisType; // Determines max. approximation order

    // Clustering strategy
    GalerkinClusteringStrategy clusteringStrategy;

    // Some global variables for form-factor computation
    GalerkinElement *formFactorLastRcv;
    GalerkinElement *formFactorLastSrc;

    // Scratch offscreen renderer for various clustering operations
    SGL_CONTEXT *scratch;
    int scratchFbSize; // Scratch frame buffer size
    int lastClusterId; // Used for caching cluster and eye point
    Vector3D lastEye; // Rendered into the scratch frame buffer

    unsigned long lastClock; // For CPU timing
    float cpu_secs;

    GalerkinState();
};

extern GalerkinState GLOBAL_galerkin_state;

#define DEFAULT_GAL_HIERARCHICAL true
#define DEFAULT_GAL_IMPORTANCE_DRIVEN false
#define DEFAULT_GAL_CLUSTERED true
#define DEFAULT_GAL_ITERATION_METHOD JACOBI
#define DEFAULT_GAL_LAZY_LINKING true
#define DEFAULT_GAL_CONSTANT_RADIANCE false
#define DEFAULT_GAL_AMBIENT_RADIANCE false
#define DEFAULT_GAL_SHAFT_CULL_MODE DO_SHAFT_CULLING_FOR_REFINEMENT
#define DEFAULT_GAL_RCV_CUBATURE_DEGREE DEGREE_5
#define DEFAULT_GAL_SRC_CUBATURE_DEGREE DEGREE_4
#define DEFAULT_GAL_REL_MIN_ELEM_AREA 1e-6
#define DEFAULT_GAL_REL_LINK_ERROR_THRESHOLD 1e-5
#define DEFAULT_GAL_ERROR_NORM POWER_ERROR
#define DEFAULT_GAL_BASIS_TYPE LINEAR
#define DEFAULT_GAL_EXACT_VISIBILITY true
#define DEFAULT_GAL_MULTI_RESOLUTION_VISIBILITY false
#define DEFAULT_GAL_CLUSTERING_STRATEGY ISOTROPIC
#define DEFAULT_GAL_SCRATCH_FB_SIZE 200

enum GalerkinRole {
    SOURCE,
    RECEIVER
};

// In GalerkinRadiosity.cpp
extern void setCubatureRules(CUBARULE **triRule, CUBARULE **quadRule, GalerkinCubatureDegree degree);
extern void patchRecomputeColor(Patch *patch);

// In shooting.cpp
extern int doShootingStep(java::ArrayList<Patch *> *scenePatches);

// In gathering.cpp
extern int randomWalkRadiosityDoGatheringIteration(java::ArrayList<Patch *> *scenePatches);
extern int doClusteredGatheringIteration(java::ArrayList<Patch *> *scenePatches);

// In initial linking.cpp
extern void createInitialLinks(GalerkinElement *top, GalerkinRole role);
extern void createInitialLinkWithTopCluster(GalerkinElement *elem, GalerkinRole role);

// In hie refine.cpp
extern void refineInteractions(GalerkinElement *top, GalerkinState state, java::ArrayList<Geometry *> **candidateOccludersList);

// In basis galerkin.cpp
extern void basisGalerkinPushPullRadiance(GalerkinElement *top);

#endif
