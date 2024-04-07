#ifndef __GALERKIN_STATE__
#define __GALERKIN_STATE__

#include "common/linealAlgebra/Vector3D.h"
#include "common/ColorRgb.h"
#include "common/cubature.h"
#include "SGL/sgl.h"
#include "GALERKIN/GalerkinElement.h"

enum GalerkinIterationMethod {
    JACOBI,
    GAUSS_SEIDEL,
    SOUTH_WELL
};

enum GalerkinShaftCullMode {
    ALWAYS_DO_SHAFT_CULLING,
    DO_SHAFT_CULLING_FOR_REFINEMENT
};

enum GalerkinCubatureDegree {
    DEGREE_1,
    DEGREE_2,
    DEGREE_3,
    DEGREE_4,
    DEGREE_5,
    DEGREE_6,
    DEGREE_7,
    DEGREE_8,
    DEGREE_9,
    DEGREE_3_PROD,
    DEGREE_5_PROD,
    DEGREE_7_PROD
};

enum GalerkinErrorNorm {
    RADIANCE_ERROR,
    POWER_ERROR
};

enum GalerkinBasisType {
    CONSTANT,
    LINEAR,
    QUADRATIC,
    CUBIC
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
    int iteration_nr; // Number of iterations and nr of steps
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
    GalerkinCubatureDegree receiverDegree;
    GalerkinCubatureDegree sourceDegree;
    CUBARULE *rcv3rule;
    CUBARULE *rcv4rule;
    CUBARULE *src3rule;
    CUBARULE *src4rule;
    CUBARULE *clusterRule;

    // Global variables concerning clustering
    GalerkinElement *topCluster; // Top level cluster containing the whole scene
    Geometry *topGeometry; // A single COMPOUND Geometry containing the whole scene

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

    // Some global variables for form-factor computation
    GalerkinElement *formFactorLastRcv;
    GalerkinElement *formFactorLastSrc;

    // Scratch offscreen renderer for various clustering operations
    SGL_CONTEXT *scratch;
    int scratchFrameBufferSize;
    int lastClusterId; // Used for caching cluster and eye point
    Vector3D lastEye; // Rendered into the scratch frame buffer

    unsigned long lastClock; // For CPU timing
    float cpuSeconds;

    GalerkinState();
};

#endif
