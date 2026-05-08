#ifndef __GALERKIN_STATE__
#define __GALERKIN_STATE__

#include "common/linealAlgebra/Vector3D.h"
#include "common/ColorRgb.h"
#include "numericalAnalysis/CubatureRule.h"
#include "skin/Geometry.h"
#include "skin/PatchSet.h"
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

    long long lastClock; // For CPU timing (nanoseconds)
    float cpuSeconds;

    ShaftCullStrategy shaftCullStrategy;
    ToneMappingContext *toneMapOptions;
    java::ArrayList<PatchSet *> *scenePatchSetList;
    java::ArrayList<PatchSet *> *clusteredPatchSetList;

    GalerkinState();

  private:
    static constexpr bool DEFAULT_GAL_HIERARCHICAL = true;
    static constexpr GalerkinIterationMethod DEFAULT_GAL_ITERATION_METHOD = GalerkinIterationMethod::JACOBI;
    static constexpr float DEFAULT_GAL_REL_MIN_ELEM_AREA = 1e-6f;
    static constexpr float DEFAULT_GAL_REL_LINK_ERROR_THRESHOLD = 1e-5f;
    static constexpr bool DEFAULT_GAL_IMPORTANCE_DRIVEN = false;
    static constexpr bool DEFAULT_GAL_CLUSTERED = true;
    static constexpr bool DEFAULT_GAL_LAZY_LINKING = true;
    static constexpr bool DEFAULT_GAL_AMBIENT_RADIANCE = false;
    static constexpr CubatureDegree DEFAULT_GAL_RCV_CUBATURE_DEGREE = CubatureDegree::DEGREE_5;
    static constexpr CubatureDegree DEFAULT_GAL_SRC_CUBATURE_DEGREE = CubatureDegree::DEGREE_4;
    static constexpr GalerkinClusteringStrategy DEFAULT_GAL_CLUSTERING_STRATEGY = GalerkinClusteringStrategy::ISOTROPIC;
    static constexpr GalerkinShaftCullMode DEFAULT_GAL_SHAFT_CULL_MODE = GalerkinShaftCullMode::DO_SHAFT_CULLING_FOR_REFINEMENT;
    static constexpr GalerkinErrorNorm DEFAULT_GAL_ERROR_NORM = GalerkinErrorNorm::POWER_ERROR;
    static constexpr GalerkinBasisType DEFAULT_GAL_BASIS_TYPE = GalerkinBasisType::GALERKIN_LINEAR;
    static constexpr bool DEFAULT_GAL_CONSTANT_RADIANCE = false;
    static constexpr bool DEFAULT_GAL_EXACT_VISIBILITY = true;
    static constexpr bool DEFAULT_GAL_MULTI_RESOLUTION_VISIBILITY = false;
    static constexpr int DEFAULT_GAL_SCRATCH_FRAME_BUFFER_SIDE_SIZE_IN_PIXELS = 200;
    static constexpr ShaftCullStrategy DEFAULT_GAL_SHAFT_CULL_STRATEGY = ShaftCullStrategy::OVERLAP_OPEN;
    static constexpr int DEFAULT_GAL_ITERATION_NOT_INITIALIZED = -1;
};

#endif
