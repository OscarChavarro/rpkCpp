#include "GALERKIN/GalerkinState.h"

// -gr-hierarchical -gr-no-hierarchical option
#define DEFAULT_GAL_HIERARCHICAL true

// -gr-iteration-method option
#define DEFAULT_GAL_ITERATION_METHOD JACOBI

// -gr-min-elem-area
#define DEFAULT_GAL_REL_MIN_ELEM_AREA 1e-6f

// -gr-link-error-threshold option
#define DEFAULT_GAL_REL_LINK_ERROR_THRESHOLD 1e-5f

// -gr-importance and -gr-no-importance options
#define DEFAULT_GAL_IMPORTANCE_DRIVEN false

// -gr-clustering and -gr-no-clustering options
#define DEFAULT_GAL_CLUSTERED true

// -gr-lazy-linking and -gr-no-lazy-linking options
#define DEFAULT_GAL_LAZY_LINKING true

// -gr-ambient and -gr-no-ambient options
#define DEFAULT_GAL_AMBIENT_RADIANCE false

// Hardcoded, not changeable via command line parameters
#define DEFAULT_GAL_RCV_CUBATURE_DEGREE DEGREE_5
#define DEFAULT_GAL_SRC_CUBATURE_DEGREE DEGREE_4
#define DEFAULT_GAL_CLUSTERING_STRATEGY GalerkinClusteringStrategy::ISOTROPIC
#define DEFAULT_GAL_SHAFT_CULL_MODE DO_SHAFT_CULLING_FOR_REFINEMENT
#define DEFAULT_GAL_ERROR_NORM POWER_ERROR
#define DEFAULT_GAL_BASIS_TYPE LINEAR
#define DEFAULT_GAL_CONSTANT_RADIANCE false
#define DEFAULT_GAL_EXACT_VISIBILITY true
#define DEFAULT_GAL_MULTI_RESOLUTION_VISIBILITY false
#define DEFAULT_GAL_SCRATCH_FRAME_BUFFER_SIDE_SIZE_IN_PIXELS 200

// Default strategy is "overlap open", which was the most efficient strategy tested
#define DEFAULT_GAL_SHAFT_CULL_STRATEGY ShaftCullStrategy::OVERLAP_OPEN

// Other Constant initial values
#define DEFAULT_GAL_ITERATION_NOT_INITIALIZED (-1)

GalerkinState::GalerkinState():
    constantRadiance(),
    ambientRadiance(),
    rcv3rule(),
    rcv4rule(),
    src3rule(),
    src4rule(),
    topCluster(),
    lastClusterId(),
    lastEye(),
    lastClock(),
    cpuSeconds()
{
    hierarchical = DEFAULT_GAL_HIERARCHICAL;
    galerkinIterationMethod = DEFAULT_GAL_ITERATION_METHOD;
    relMinElemArea = DEFAULT_GAL_REL_MIN_ELEM_AREA;
    relLinkErrorThreshold = DEFAULT_GAL_REL_LINK_ERROR_THRESHOLD;
    importanceDriven = DEFAULT_GAL_IMPORTANCE_DRIVEN;
    clustered = DEFAULT_GAL_CLUSTERED;
    lazyLinking = DEFAULT_GAL_LAZY_LINKING;
    useAmbientRadiance = DEFAULT_GAL_AMBIENT_RADIANCE;

    receiverDegree = DEFAULT_GAL_RCV_CUBATURE_DEGREE;
    sourceDegree = DEFAULT_GAL_SRC_CUBATURE_DEGREE;
    clusteringStrategy = DEFAULT_GAL_CLUSTERING_STRATEGY;
    shaftCullMode = DEFAULT_GAL_SHAFT_CULL_MODE;
    errorNorm = DEFAULT_GAL_ERROR_NORM;
    basisType = DEFAULT_GAL_BASIS_TYPE;
    useConstantRadiance = DEFAULT_GAL_CONSTANT_RADIANCE;
    exactVisibility = DEFAULT_GAL_EXACT_VISIBILITY;
    multiResolutionVisibility = DEFAULT_GAL_MULTI_RESOLUTION_VISIBILITY;
    scratchFrameBufferSize = DEFAULT_GAL_SCRATCH_FRAME_BUFFER_SIDE_SIZE_IN_PIXELS;
    scratch = nullptr;
    iterationNumber = DEFAULT_GAL_ITERATION_NOT_INITIALIZED;
    shaftCullStrategy = DEFAULT_GAL_SHAFT_CULL_STRATEGY;

    setCubatureRules(&rcv3rule, &rcv4rule, receiverDegree);
    setCubatureRules(&src3rule, &src4rule, sourceDegree);
    clusterRule = &GLOBAL_crv1;
}
