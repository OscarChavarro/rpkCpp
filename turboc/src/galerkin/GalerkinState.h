#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __GALERKIN_STATE__
#define __GALERKIN_STATE__

#include "common/linealAlgebra/Vector3D.h"
#include "common/ColorRgb.h"
#include "numericalAnalysis/CubatureRule.h"
#include "skin/Geometry.h"
#include "render/sgl/SglContext.h"
#include "galerkin/GalerkinBasisType.h"
#include "galerkin/GalerkinClusteringStrategy.h"
#include "galerkin/GalerkinElement.h"
#include "galerkin/GalerkinErrorNorm.h"
#include "galerkin/GalerkinIterationMethod.h"
#include "galerkin/GalerkinShaftCullMode.h"
#include "galerkin/ShaftCullStrategy.h"
#include "tonemap/ToneMappingContext.h"

class GalerkinState {
  public:
    int iterationNumber;
    bool hierarchical; // Set true for hierarchical refinement
    int importanceDriven; // Set true for potential-driven comp
    int clustered; // Set true for clustering
    GalerkinIterationMethod galerkinIterationMethod; // How to solve the resulting linear set
    int lazyLinking; // Set true for lazy linking
    int exactVisibility; // For more exact treatment of visibility
    int multiResolutionVisibility; // For multi-resolution visibility determination
    int useConstantRadiance; // Set true for constant radiance initialization
    int useAmbientRadiance; // Ambient radiance (for visualisation only)
    ColorRgb constantRadiance;
    ColorRgb ambientRadiance;
    GalerkinShaftCullMode shaftCullMode; // When to do shaft culling

    // Cubature rules for computing form factors
    CubatureDegree receiverDegree;
    CubatureDegree sourceDegree;
    CubatureRule *receiverTriangleCubatureRule;
    CubatureRule *receiverQuadCubatureRule;
    CubatureRule *sourceTriangleCubatureRule;
    CubatureRule *sourceQuadCubatureRule;
    CubatureRule *clusterRule;

    // Global variables concerning clustering
    GalerkinElement *topCluster; // Top level cluster containing the whole scene

    // Parameters that control accuracy
    GalerkinErrorNorm errorNorm; // Control radiance or power error?
    float relMinElemArea; // Subdivision of elements that are smaller than the total
        // surface area of the scene times this number, will not be allowed
    float relLinkErrorThreshold;  // Relative to maximum self-emitted radiance
        // when controlling the radiance error and to the maximum
        // self-emitted power when controlling the power error

    GalerkinBasisType basisType; // Determines max. approximation order

    // Clustering strategy
    GalerkinClusteringStrategy clusteringStrategy;

    // Scratch offscreen renderer for various clustering operations
    SglContext *scratch;
    int scratchFrameBufferSize;
    int lastClusterId; // Used for caching cluster and eye point
    Vector3D lastEye; // Rendered into the scratch frame buffer

    long lastClock; // For CPU timing (nanoseconds)
    float cpuSeconds;

    ShaftCullStrategy shaftCullStrategy;
    ToneMappingContext *toneMapOptions;

    GalerkinState();

  private:
    #define DEFAULT_GAL_HIERARCHICAL true
    #define DEFAULT_GAL_ITERATION_METHOD JACOBI
    #define DEFAULT_GAL_REL_MIN_ELEM_AREA ((float)1e-6)
    #define DF_GAL_REL_LINK_ERRR_THRSH ((float)1e-5)
    #define DEFAULT_GAL_IMPORTANCE_DRIVEN false
    #define DEFAULT_GAL_CLUSTERED true
    #define DEFAULT_GAL_LAZY_LINKING true
    #define DEFAULT_GAL_AMBIENT_RADIANCE false
    #define DEFAULT_GAL_RCV_CUBATURE_DEGREE DEGREE_5
    #define DEFAULT_GAL_SRC_CUBATURE_DEGREE DEGREE_4
    #define DEFAULT_GAL_CLUSTERING_STRATEGY ISOTROPIC
    #define DEFAULT_GAL_SHAFT_CULL_MODE DO_SHAFT_CULLING_FOR_REFINEMENT
    #define DEFAULT_GAL_ERROR_NORM POWER_ERROR
    #define DEFAULT_GAL_BASIS_TYPE GALERKIN_LINEAR
    #define DEFAULT_GAL_CONSTANT_RADIANCE false
    #define DEFAULT_GAL_EXACT_VISIBILITY true
    #define DF_GAL_MLT_RSLTN_VSBLT false
    #define DF_GAL_SCR_FRM_BUF_SID_SIZ_I_PX 200
    #define DEFAULT_GAL_SHAFT_CULL_STRATEGY OVERLAP_OPEN
    #define DF_GAL_ITRTN_NOT_INTLZ -1
};

#endif
