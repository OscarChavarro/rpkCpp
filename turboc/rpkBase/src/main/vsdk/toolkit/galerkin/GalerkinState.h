#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __GALERKIN_STATE__
#define __GALERKIN_STATE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/numericalAnalysis/CubatureRule.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
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

class ScratchVisibilityWorkspace;

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
    double relMinElemArea; // Subdivision of elements that are smaller than the total
        // surface area of the scene times this number, will not be allowed
    double relLinkErrorThreshold;  // Relative to maximum self-emitted radiance
        // when controlling the radiance error and to the maximum
        // self-emitted power when controlling the power error
    double refinementMinimumArea;
    double refinementRadianceErrorThreshold;
    double refinementPowerErrorThresholdNumerator;
    double refinementMaxDirectPotential;

    GalerkinBasisType basisType; // Determines max. approximation order

    // Clustering strategy
    GalerkinClusteringStrategy clusteringStrategy;

    // Scratch offscreen renderer of the solver for the Z-visibility
    // clustering strategy
    ScratchVisibilityWorkspace *scratchVisibilityWorkspace;
    int scratchFrameBufferSize;

    long long lastClock; // For CPU timing (nanoseconds)
    double cpuSeconds;

    ShaftCullStrategy shaftCullStrategy;
    ToneMappingContext *toneMapOptions;

    // Form factor scratch owned by the solver
    FormFactorWorkspace *formFactorWorkspace;

    GalerkinState();
    ~GalerkinState();

    /** Jacobi iterations: gathering with the radiance of the previous iteration */
    inline bool isJacobi() const;

    const RadianceElementTable *getRadianceElements() const;
    GalerkinElement *topLevelElement(const Patch *patch) const;
    const Element *boundElement(const Patch *patch) const;
    void setRadianceElements(const RadianceElementTable *inRadianceElements);

  private:
    // Not copyable
    GalerkinState(const GalerkinState &);
    GalerkinState &operator=(const GalerkinState &);

    // The scene's table of top-level elements, bound when the method is initialized
    const RadianceElementTable *radianceElements;

    #define DEFAULT_GAL_HIERARCHICAL true
    #define DEFAULT_GAL_ITERATION_METHOD JACOBI
    #define DEFAULT_GAL_REL_MIN_ELEM_AREA ((double)1e-6)
    #define DF_GAL_REL_LINK_ERRR_THRSH ((double)1e-5)
    #define DEFAULT_GAL_IMPORTANCE_DRIVEN false
    #define DEFAULT_GAL_CLUSTERED true
    #define DEFAULT_GAL_LAZY_LINKING true
    #define DEFAULT_GAL_AMBIENT_RADIANCE false
    #define DEFAULT_GAL_RCV_CUBATURE_DEGREE DEGREE_5
    #define DEFAULT_GAL_SRC_CUBATURE_DEGREE DEGREE_4
    #define DEFAULT_GAL_CLUSTERING_STRATEGY ISOTROPIC
    #define DEFAULT_GAL_SHAFT_CULL_MODE NEVER_DO_SHAFT_CULLING
    #define DEFAULT_GAL_ERROR_NORM POWER_ERROR
    #define DEFAULT_GAL_BASIS_TYPE GALERKIN_LINEAR
    #define DEFAULT_GAL_CONSTANT_RADIANCE false
    #define DEFAULT_GAL_EXACT_VISIBILITY true
    #define DF_GAL_MLT_RSLTN_VSBLT false
    #define DF_GAL_SCR_FRM_BUF_SID_SIZ_I_PX 200
    #define DEFAULT_GAL_SHAFT_CULL_STRATEGY OVERLAP_OPEN
    #define DF_GAL_ITRTN_NOT_INTLZ -1
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
    return ((GalerkinElement *)(radianceElements->get(patch->getFaceIndex())));
}

/**
Element bound to a face, of any radiance method, or NULL when no method is
bound or the face has no element (checked lookup for tools)
*/
inline const Element *
GalerkinState::boundElement(const Patch *patch) const {
    if ( radianceElements == NULL || patch == NULL ) {
        return NULL;
    }
    return radianceElements->get(patch->getFaceIndex());
}

/**
The cpp port's PARALLEL_JACOBI is Jacobi too; the serial Turbo C++ port has
JACOBI only
*/
inline bool
GalerkinState::isJacobi() const {
    return galerkinIterationMethod == JACOBI;
}

#endif
