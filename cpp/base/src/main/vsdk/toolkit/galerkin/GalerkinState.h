#ifndef GALERKIN_STATE__
#define GALERKIN_STATE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/numericalAnalysis/CubatureRule.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
#include "vsdk/toolkit/render/sgl/SglContext.h"
#include "vsdk/toolkit/galerkin/GalerkinBasisType.h"
#include "vsdk/toolkit/galerkin/GalerkinClusteringStrategy.h"
#include "vsdk/toolkit/galerkin/GalerkinElement.h"
#include "vsdk/toolkit/galerkin/GalerkinErrorNorm.h"
#include "vsdk/toolkit/galerkin/GalerkinIterationMethod.h"
#include "vsdk/toolkit/galerkin/GalerkinShaftCullMode.h"
#include "vsdk/toolkit/galerkin/ShaftCullStrategy.h"
#include "vsdk/toolkit/galerkin/processing/FormFactorWorkspace.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"
#include "vsdk/toolkit/scene/RadianceElementTable.h"

class GalerkinState {
  public:
    int iterationNumber;
    bool hierarchical; // Set true for hierarchical refinement
    int importanceDriven; // Set true for potential-driven comp
    int clustered; // Set true for clustering
    GalerkinIterationMethod galerkinIterationMethod; // How to solve the resulting linear set
    int numberOfThreads; // Threads of PARALLEL_JACOBI; 0 for one per online processor
    int lazyLinking; // Set true for lazy linking
    int exactVisibility; // For more exact treatment of visibility
    int multiResolutionVisibility; // For multi-resolution visibility determination
    int useConstantRadiance; // Set true for constant radiance initialization
    int useAmbientRadiance; // Ambient radiance (for visualisation only)
    ColorRgbMutable constantRadiance;
    ColorRgbMutable ambientRadiance;
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
    double refinementMinimumArea;
    double refinementRadianceErrorThreshold;
    double refinementPowerErrorThresholdNumerator;
    double refinementMaxDirectPotential;

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

    // Form factor scratch owned by the solver
    FormFactorWorkspace *formFactorWorkspace;

    GalerkinState();
    ~GalerkinState();
    GalerkinState(const GalerkinState &) = delete;
    GalerkinState &operator=(const GalerkinState &) = delete;

    /** Jacobi iterations, serial or parallel: both gather with the radiance of the previous iteration */
    bool isJacobi() const;

    const RadianceElementTable *getRadianceElements() const;
    GalerkinElement *topLevelElement(const Patch *patch) const;
    const Element *boundElement(const Patch *patch) const;
    void setRadianceElements(const RadianceElementTable *inRadianceElements);

  private:
    // The scene's table of top-level elements, bound when the method is initialized
    const RadianceElementTable *radianceElements;
    static constexpr bool DEFAULT_GAL_HIERARCHICAL = true;
    static constexpr GalerkinIterationMethod DEFAULT_GAL_ITERATION_METHOD = GalerkinIterationMethod::JACOBI;
    static constexpr float DEFAULT_GAL_REL_MIN_ELEM_AREA = 1e-6F;
    static constexpr float DEFAULT_GAL_REL_LINK_ERROR_THRESHOLD = 1e-5F;
    static constexpr bool DEFAULT_GAL_IMPORTANCE_DRIVEN = false;
    static constexpr bool DEFAULT_GAL_CLUSTERED = true;
    static constexpr bool DEFAULT_GAL_LAZY_LINKING = true;
    static constexpr bool DEFAULT_GAL_AMBIENT_RADIANCE = false;
    static constexpr CubatureDegree DEFAULT_GAL_RCV_CUBATURE_DEGREE = CubatureDegree::DEGREE_5;
    static constexpr CubatureDegree DEFAULT_GAL_SRC_CUBATURE_DEGREE = CubatureDegree::DEGREE_4;
    static constexpr GalerkinClusteringStrategy DEFAULT_GAL_CLUSTERING_STRATEGY = GalerkinClusteringStrategy::ISOTROPIC;
    static constexpr GalerkinShaftCullMode DEFAULT_GAL_SHAFT_CULL_MODE = GalerkinShaftCullMode::NEVER_DO_SHAFT_CULLING;
    static constexpr GalerkinErrorNorm DEFAULT_GAL_ERROR_NORM = GalerkinErrorNorm::POWER_ERROR;
    static constexpr GalerkinBasisType DEFAULT_GAL_BASIS_TYPE = GalerkinBasisType::GALERKIN_LINEAR;
    static constexpr bool DEFAULT_GAL_CONSTANT_RADIANCE = false;
    static constexpr bool DEFAULT_GAL_EXACT_VISIBILITY = true;
    static constexpr bool DEFAULT_GAL_MULTI_RESOLUTION_VISIBILITY = false;
    static constexpr int DEFAULT_GAL_SCRATCH_FRAME_BUFFER_SIDE_SIZE_IN_PIXELS = 200;
    static constexpr ShaftCullStrategy DEFAULT_GAL_SHAFT_CULL_STRATEGY = ShaftCullStrategy::OVERLAP_OPEN;
    static constexpr int DEFAULT_GAL_ITERATION_NOT_INITIALIZED = -1;
};

inline const RadianceElementTable *
GalerkinState::getRadianceElements() const {
    return radianceElements;
}

inline void
GalerkinState::setRadianceElements(const RadianceElementTable *inRadianceElements) {
    radianceElements = inRadianceElements;
}

/**
Top-level element of a face, without checks (hot path); see
GalerkinElement::fromPatch for the checked lookup
*/
inline GalerkinElement *
GalerkinState::topLevelElement(const Patch *patch) const {
    return static_cast<GalerkinElement *>(radianceElements->get(patch->getFaceIndex()));
}

/**
Element bound to a face, of any radiance method, or nullptr when no method is
bound or the face has no element (checked lookup for tools)
*/
inline const Element *
GalerkinState::boundElement(const Patch *patch) const {
    if ( radianceElements == nullptr || patch == nullptr ) {
        return nullptr;
    }
    return radianceElements->get(patch->getFaceIndex());
}

inline bool
GalerkinState::isJacobi() const {
    return galerkinIterationMethod == GalerkinIterationMethod::JACOBI
        || galerkinIterationMethod == GalerkinIterationMethod::PARALLEL_JACOBI;
}

#endif
