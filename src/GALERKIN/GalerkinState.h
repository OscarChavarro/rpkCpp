#ifndef __GALERKIN_STATE__
#define __GALERKIN_STATE__

#include "common/linealAlgebra/Vector3D.h"
#include "common/ColorRgb.h"
#include "common/numericalAnalysis/CubatureRule.h"
#include "SGL/sgl.h"
#include "GALERKIN/GalerkinElement.h"
#include "GALERKIN/ShaftCullStrategy.h"

/**
Reference:
[COHE1993] Cohen, M. Wallace, J. "Radiosity and Realistic Image Synthesis",
     Academic Press Professional, 1993.
*/

// See [COHE1993] section 5.3.
enum GalerkinIterationMethod {
    JACOBI,
    GAUSS_SEIDEL,
    SOUTH_WELL
};

enum GalerkinShaftCullMode {
    ALWAYS_DO_SHAFT_CULLING,
    DO_SHAFT_CULLING_FOR_REFINEMENT
};

enum GalerkinErrorNorm {
    RADIANCE_ERROR,
    POWER_ERROR
};

enum GalerkinBasisType {
    GALERKIN_CONSTANT,
    GALERKIN_LINEAR,
    GALERKIN_QUADRATIC,
    GALERKIN_CUBIC
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
    int iterationNumber;
    int hierarchical; // Set true for hierarchical refinement
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
    SGL_CONTEXT *scratch;
    int scratchFrameBufferSize;
    int lastClusterId; // Used for caching cluster and eye point
    Vector3D lastEye; // Rendered into the scratch frame buffer

    unsigned long lastClock; // For CPU timing
    float cpuSeconds;

    ShaftCullStrategy shaftCullStrategy;

    GalerkinState();
};

#endif
