#include "numericalAnalysis/QuadCubatureRule.h"
#include "numericalAnalysis/TriangleCubatureRule.h"
#include "galerkin/GalerkinState.h"

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
    cpuSeconds(),
    toneMapOptions(),
    scenePatchSetList(),
    clusteredPatchSetList()
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
    toneMapOptions = nullptr;
    scenePatchSetList = nullptr;
    clusteredPatchSetList = nullptr;

    TriangleCubatureRule::setTriangleCubatureRules(&receiverTriangleCubatureRule, receiverDegree);
    TriangleCubatureRule::setTriangleCubatureRules(&sourceTriangleCubatureRule, sourceDegree);
    QuadCubatureRule::setQuadCubatureRules(&receiverQuadCubatureRule, receiverDegree);
    QuadCubatureRule::setQuadCubatureRules(&sourceQuadCubatureRule, sourceDegree);
    clusterRule = QuadCubatureRule::degree1BoxRule();
}
