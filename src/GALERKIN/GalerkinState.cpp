#include "common/numericalAnalysis/TriangleCubatureRule.h"
#include "common/numericalAnalysis/QuadCubatureRule.h"
#include "GALERKIN/GalerkinState.h"

// -gr-hierarchical -gr-no-hierarchical option
static const bool DEFAULT_GAL_HIERARCHICAL = true;

// -gr-iteration-method option
static const GalerkinIterationMethod DEFAULT_GAL_ITERATION_METHOD = GalerkinIterationMethod::JACOBI;

// -gr-min-elem-area
static const float DEFAULT_GAL_REL_MIN_ELEM_AREA = 1e-6f;

// -gr-link-error-threshold option
static const float DEFAULT_GAL_REL_LINK_ERROR_THRESHOLD = 1e-5f;

// -gr-importance and -gr-no-importance options
static const bool DEFAULT_GAL_IMPORTANCE_DRIVEN = false;

// -gr-clustering and -gr-no-clustering options
static const bool DEFAULT_GAL_CLUSTERED = true;

// -gr-lazy-linking and -gr-no-lazy-linking options
static const bool DEFAULT_GAL_LAZY_LINKING = true;

// -gr-ambient and -gr-no-ambient options
static const bool DEFAULT_GAL_AMBIENT_RADIANCE = false;

// Hardcoded, not changeable via command line parameters
static const CubatureDegree DEFAULT_GAL_RCV_CUBATURE_DEGREE = CubatureDegree::DEGREE_5;
static const CubatureDegree DEFAULT_GAL_SRC_CUBATURE_DEGREE = CubatureDegree::DEGREE_4;
static const GalerkinClusteringStrategy DEFAULT_GAL_CLUSTERING_STRATEGY = GalerkinClusteringStrategy::ISOTROPIC;
static const GalerkinShaftCullMode DEFAULT_GAL_SHAFT_CULL_MODE = GalerkinShaftCullMode::DO_SHAFT_CULLING_FOR_REFINEMENT;
static GalerkinErrorNorm DEFAULT_GAL_ERROR_NORM = GalerkinErrorNorm::POWER_ERROR;
static const GalerkinBasisType DEFAULT_GAL_BASIS_TYPE = GalerkinBasisType::LINEAR;
static const bool DEFAULT_GAL_CONSTANT_RADIANCE = false;
static const bool DEFAULT_GAL_EXACT_VISIBILITY = true;
static const bool DEFAULT_GAL_MULTI_RESOLUTION_VISIBILITY = false;
static const int DEFAULT_GAL_SCRATCH_FRAME_BUFFER_SIDE_SIZE_IN_PIXELS = 200;

// Default strategy is "overlap open", which was the most efficient strategy tested
static const ShaftCullStrategy DEFAULT_GAL_SHAFT_CULL_STRATEGY = ShaftCullStrategy::OVERLAP_OPEN;

// Other Constant initial values
static const int DEFAULT_GAL_ITERATION_NOT_INITIALIZED = -1;

GalerkinState::GalerkinState():
    constantRadiance(),
    ambientRadiance(),
    receiverTriangleCubatureRule(),
    receiverQuadCubatureRule(),
    sourceTriangleCubatureRule(),
    sourceQuadCubatureRule(),
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

    TriangleCubatureRule::setTriangleCubatureRules(&receiverTriangleCubatureRule, receiverDegree);
    TriangleCubatureRule::setTriangleCubatureRules(&sourceTriangleCubatureRule, sourceDegree);
    QuadCubatureRule::setQuadCubatureRules(&receiverQuadCubatureRule, receiverDegree);
    QuadCubatureRule::setQuadCubatureRules(&sourceQuadCubatureRule, sourceDegree);
    clusterRule = &GLOBAL_crv1;
}
