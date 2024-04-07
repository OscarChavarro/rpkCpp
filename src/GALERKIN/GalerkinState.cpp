#include "common/error.h"
#include "GALERKIN/GalerkinState.h"

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

/**
Installs cubature rules for triangles and quadrilaterals of the specified degree
*/
static void
setCubatureRules(CUBARULE **triRule, CUBARULE **quadRule, GalerkinCubatureDegree degree) {
    switch ( degree ) {
        case DEGREE_1:
            *triRule = &GLOBAL_crt1;
            *quadRule = &GLOBAL_crq1;
            break;
        case DEGREE_2:
            *triRule = &GLOBAL_crt2;
            *quadRule = &GLOBAL_crq2;
            break;
        case DEGREE_3:
            *triRule = &GLOBAL_crt3;
            *quadRule = &GLOBAL_crq3;
            break;
        case DEGREE_4:
            *triRule = &GLOBAL_crt4;
            *quadRule = &GLOBAL_crq4;
            break;
        case DEGREE_5:
            *triRule = &GLOBAL_crt5;
            *quadRule = &GLOBAL_crq5;
            break;
        case DEGREE_6:
            *triRule = &GLOBAL_crt7;
            *quadRule = &GLOBAL_crq6;
            break;
        case DEGREE_7:
            *triRule = &GLOBAL_crt7;
            *quadRule = &GLOBAL_crq7;
            break;
        case DEGREE_8:
            *triRule = &GLOBAL_crt8;
            *quadRule = &GLOBAL_crq8;
            break;
        case DEGREE_9:
            *triRule = &GLOBAL_crt9;
            *quadRule = &GLOBAL_crq9;
            break;
        case DEGREE_3_PROD:
            *triRule = &GLOBAL_crt5;
            *quadRule = &GLOBAL_crq3pg;
            break;
        case DEGREE_5_PROD:
            *triRule = &GLOBAL_crt7;
            *quadRule = &GLOBAL_crq5pg;
            break;
        case DEGREE_7_PROD:
            *triRule = &GLOBAL_crt9;
            *quadRule = &GLOBAL_crq7pg;
            break;
        default:
            logFatal(2, "setCubatureRules", "Invalid degree %d", degree);
    }
}

GalerkinState::GalerkinState():
        iterationNumber(),
        hierarchical(),
        importanceDriven(),
        clustered(),
        galerkinIterationMethod(),
        lazyLinking(),
        exactVisibility(),
        multiResolutionVisibility(),
        useConstantRadiance(),
        useAmbientRadiance(),
        constantRadiance(),
        ambientRadiance(),
        shaftCullMode(),
        receiverDegree(),
        sourceDegree(),
        rcv3rule(),
        rcv4rule(),
        src3rule(),
        src4rule(),
        clusterRule(),
        topCluster(),
        topGeometry(),
        errorNorm(),
        relMinElemArea(),
        relLinkErrorThreshold (),
        basisType(),
        clusteringStrategy(),
        formFactorLastRcv(),
        formFactorLastSrc(),
        scratch(),
        scratchFrameBufferSize(),
        lastClusterId(),
        lastEye(),
        lastClock(),
        cpuSeconds()
{
    hierarchical = DEFAULT_GAL_HIERARCHICAL;
    importanceDriven = DEFAULT_GAL_IMPORTANCE_DRIVEN;
    clustered = DEFAULT_GAL_CLUSTERED;
    galerkinIterationMethod = DEFAULT_GAL_ITERATION_METHOD;
    lazyLinking = DEFAULT_GAL_LAZY_LINKING;
    useConstantRadiance = DEFAULT_GAL_CONSTANT_RADIANCE;
    useAmbientRadiance = DEFAULT_GAL_AMBIENT_RADIANCE;
    shaftCullMode = DEFAULT_GAL_SHAFT_CULL_MODE;
    receiverDegree = DEFAULT_GAL_RCV_CUBATURE_DEGREE;
    sourceDegree = DEFAULT_GAL_SRC_CUBATURE_DEGREE;
    setCubatureRules(&rcv3rule, &rcv4rule, receiverDegree);
    setCubatureRules(&src3rule, &src4rule, sourceDegree);
    clusterRule = &GLOBAL_crv1;
    relMinElemArea = DEFAULT_GAL_REL_MIN_ELEM_AREA;
    relLinkErrorThreshold = DEFAULT_GAL_REL_LINK_ERROR_THRESHOLD;
    errorNorm = DEFAULT_GAL_ERROR_NORM;
    basisType = DEFAULT_GAL_BASIS_TYPE;
    exactVisibility = DEFAULT_GAL_EXACT_VISIBILITY;
    multiResolutionVisibility = DEFAULT_GAL_MULTI_RESOLUTION_VISIBILITY;
    clusteringStrategy = DEFAULT_GAL_CLUSTERING_STRATEGY;
    scratch = nullptr;
    scratchFrameBufferSize = DEFAULT_GAL_SCRATCH_FB_SIZE;
    iterationNumber = -1; // This means "not initialized"
}
