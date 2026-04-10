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
    toneMapOptions()
{
    hierarchical = DEFAULT_GAL_HIERARCHICAL;
    galerkinIterationMethod = DEFAULT_GAL_ITERATION_METHOD;
    relMinElemArea = DEFAULT_GAL_REL_MIN_ELEM_AREA;
    relLinkErrorThreshold = DF_GAL_REL_LINK_ERRR_THRSH;
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
    multiResolutionVisibility = DF_GAL_MLT_RSLTN_VSBLT;
    scratchFrameBufferSize = DF_GAL_SCR_FRM_BUF_SID_SIZ_I_PX;
    scratch = NULL;
    iterationNumber = DF_GAL_ITRTN_NOT_INTLZ;
    shaftCullStrategy = DEFAULT_GAL_SHAFT_CULL_STRATEGY;
    toneMapOptions = NULL;

    TriangleCubatureRule::setTriangleCubatureRules(&receiverTriangleCubatureRule, receiverDegree);
    TriangleCubatureRule::setTriangleCubatureRules(&sourceTriangleCubatureRule, sourceDegree);
    QuadCubatureRule::setQuadCubatureRules(&receiverQuadCubatureRule, receiverDegree);
    QuadCubatureRule::setQuadCubatureRules(&sourceQuadCubatureRule, sourceDegree);
    clusterRule = QuadCubatureRule::degree1BoxRule();
}
