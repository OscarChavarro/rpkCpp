package vsdk.toolkit.galerkin;

import vsdk.toolkit.environment.geometry.elements.Element;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.scene.RadianceElementTable;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.galerkin.processing.FormFactorWorkspace;
import vsdk.toolkit.galerkin.processing.ScratchVisibilityWorkspace;
import vsdk.toolkit.numericalAnalysis.CubatureDegree;
import vsdk.toolkit.numericalAnalysis.CubatureRule;
import vsdk.toolkit.numericalAnalysis.QuadCubatureRule;
import vsdk.toolkit.numericalAnalysis.TriangleCubatureRule;
import vsdk.toolkit.tonemap.ToneMappingContext;

public class GalerkinState {
    public int iterationNumber;
    public boolean hierarchical; // Set true for hierarchical refinement
    public int importanceDriven; // Set true for potential-driven comp
    public int clustered; // Set true for clustering
    public GalerkinIterationMethod galerkinIterationMethod; // How to solve the resulting linear set
    public int numberOfThreads; // Threads of PARALLEL_JACOBI; 0 for one per online processor
    public int lazyLinking; // Set true for lazy linking
    public int exactVisibility; // For more exact treatment of visibility
    public int multiResolutionVisibility; // For multi-resolution visibility determination
    public int useConstantRadiance; // Set true for constant radiance initialization
    public int useAmbientRadiance; // Ambient radiance (for visualisation only)
    public ColorRgb constantRadiance;
    public ColorRgb ambientRadiance;
    public GalerkinShaftCullMode shaftCullMode; // When to do shaft culling

    // Cubature rules for computing form factors
    public CubatureDegree receiverDegree;
    public CubatureDegree sourceDegree;
    public CubatureRule receiverTriangleCubatureRule;
    public CubatureRule receiverQuadCubatureRule;
    public CubatureRule sourceTriangleCubatureRule;
    public CubatureRule sourceQuadCubatureRule;
    public CubatureRule clusterRule;

    // Global variables concerning clustering
    public GalerkinElement topCluster; // Top level cluster containing the whole scene

    // Parameters that control accuracy
    public GalerkinErrorNorm errorNorm; // Control radiance or power error?
    public double relMinElemArea; // Subdivision of elements that are smaller than the total
                                 // surface area of the scene times this number, will not be allowed
    public double relLinkErrorThreshold;  // Relative to maximum self-emitted radiance
                                         // when controlling the radiance error and to the maximum
                                         // self-emitted power when controlling the power error

    // Cached once per refineInteractions() call so hierarchic refinement does not
    // repeat the same Statistics lookups and error-threshold arithmetic for every interaction
    public double refinementMinimumArea;
    public double refinementRadianceErrorThreshold;
    public double refinementPowerErrorThresholdNumerator;
    public double refinementMaxDirectPotential;

    public GalerkinBasisType basisType; // Determines max. approximation order

    // Clustering strategy
    public GalerkinClusteringStrategy clusteringStrategy;

    // Scratch offscreen renderer of the serial solver for the Z-visibility
    // clustering strategy (parallel Jacobi workers have their own)
    public ScratchVisibilityWorkspace scratchVisibilityWorkspace;
    public int scratchFrameBufferSize;

    public long lastClock; // For CPU timing (nanoseconds)
    public double cpuSeconds;

    public ShaftCullStrategy shaftCullStrategy;
    public ToneMappingContext toneMapOptions;

    // Form factor scratch owned by the solver
    public final FormFactorWorkspace formFactorWorkspace = new FormFactorWorkspace(Statistics.instance().shadow);

    private static final boolean DEFAULT_GAL_HIERARCHICAL = true;
    private static final GalerkinIterationMethod DEFAULT_GAL_ITERATION_METHOD = GalerkinIterationMethod.JACOBI;
    private static final double DEFAULT_GAL_REL_MIN_ELEM_AREA = 1e-6;
    private static final double DEFAULT_GAL_REL_LINK_ERROR_THRESHOLD = 1e-5;
    private static final boolean DEFAULT_GAL_IMPORTANCE_DRIVEN = false;
    private static final boolean DEFAULT_GAL_CLUSTERED = true;
    private static final boolean DEFAULT_GAL_LAZY_LINKING = true;
    private static final boolean DEFAULT_GAL_AMBIENT_RADIANCE = false;
    private static final CubatureDegree DEFAULT_GAL_RCV_CUBATURE_DEGREE = CubatureDegree.DEGREE_5;
    private static final CubatureDegree DEFAULT_GAL_SRC_CUBATURE_DEGREE = CubatureDegree.DEGREE_4;
    private static final GalerkinClusteringStrategy DEFAULT_GAL_CLUSTERING_STRATEGY = GalerkinClusteringStrategy.ISOTROPIC;
    private static final GalerkinShaftCullMode DEFAULT_GAL_SHAFT_CULL_MODE = GalerkinShaftCullMode.NEVER_DO_SHAFT_CULLING;
    private static final GalerkinErrorNorm DEFAULT_GAL_ERROR_NORM = GalerkinErrorNorm.POWER_ERROR;
    private static final GalerkinBasisType DEFAULT_GAL_BASIS_TYPE = GalerkinBasisType.GALERKIN_LINEAR;
    private static final boolean DEFAULT_GAL_CONSTANT_RADIANCE = false;
    private static final boolean DEFAULT_GAL_EXACT_VISIBILITY = true;
    private static final boolean DEFAULT_GAL_MULTI_RESOLUTION_VISIBILITY = false;
    private static final int DEFAULT_GAL_SCRATCH_FRAME_BUFFER_SIDE_SIZE_IN_PIXELS = 200;
    private static final ShaftCullStrategy DEFAULT_GAL_SHAFT_CULL_STRATEGY = ShaftCullStrategy.OVERLAP_OPEN;
    private static final int DEFAULT_GAL_ITERATION_NOT_INITIALIZED = -1;

    public GalerkinState() {
        constantRadiance = new ColorRgb();
        ambientRadiance = new ColorRgb();
        receiverTriangleCubatureRule = null;
        receiverQuadCubatureRule = null;
        sourceTriangleCubatureRule = null;
        sourceQuadCubatureRule = null;
        topCluster = null;
        lastClock = 0;
        cpuSeconds = 0.0;
        toneMapOptions = null;

        hierarchical = DEFAULT_GAL_HIERARCHICAL;
        galerkinIterationMethod = DEFAULT_GAL_ITERATION_METHOD;
        numberOfThreads = 0;
        relMinElemArea = DEFAULT_GAL_REL_MIN_ELEM_AREA;
        relLinkErrorThreshold = DEFAULT_GAL_REL_LINK_ERROR_THRESHOLD;
        refinementMinimumArea = 0.0;
        refinementRadianceErrorThreshold = 0.0;
        refinementPowerErrorThresholdNumerator = 0.0;
        refinementMaxDirectPotential = 0.0;
        importanceDriven = DEFAULT_GAL_IMPORTANCE_DRIVEN ? 1 : 0;
        clustered = DEFAULT_GAL_CLUSTERED ? 1 : 0;
        lazyLinking = DEFAULT_GAL_LAZY_LINKING ? 1 : 0;
        useAmbientRadiance = DEFAULT_GAL_AMBIENT_RADIANCE ? 1 : 0;

        receiverDegree = DEFAULT_GAL_RCV_CUBATURE_DEGREE;
        sourceDegree = DEFAULT_GAL_SRC_CUBATURE_DEGREE;
        clusteringStrategy = DEFAULT_GAL_CLUSTERING_STRATEGY;
        shaftCullMode = DEFAULT_GAL_SHAFT_CULL_MODE;
        errorNorm = DEFAULT_GAL_ERROR_NORM;
        basisType = DEFAULT_GAL_BASIS_TYPE;
        useConstantRadiance = DEFAULT_GAL_CONSTANT_RADIANCE ? 1 : 0;
        exactVisibility = DEFAULT_GAL_EXACT_VISIBILITY ? 1 : 0;
        multiResolutionVisibility = DEFAULT_GAL_MULTI_RESOLUTION_VISIBILITY ? 1 : 0;
        scratchFrameBufferSize = DEFAULT_GAL_SCRATCH_FRAME_BUFFER_SIDE_SIZE_IN_PIXELS;
        scratchVisibilityWorkspace = null;
        iterationNumber = DEFAULT_GAL_ITERATION_NOT_INITIALIZED;
        shaftCullStrategy = DEFAULT_GAL_SHAFT_CULL_STRATEGY;
        toneMapOptions = null;

        CubatureRule[] triRuleRef = new CubatureRule[1];
        CubatureRule[] quadRuleRef = new CubatureRule[1];

        TriangleCubatureRule.setTriangleCubatureRules(triRuleRef, receiverDegree);
        receiverTriangleCubatureRule = triRuleRef[0];
        TriangleCubatureRule.setTriangleCubatureRules(triRuleRef, sourceDegree);
        sourceTriangleCubatureRule = triRuleRef[0];
        QuadCubatureRule.setQuadCubatureRules(quadRuleRef, receiverDegree);
        receiverQuadCubatureRule = quadRuleRef[0];
        QuadCubatureRule.setQuadCubatureRules(quadRuleRef, sourceDegree);
        sourceQuadCubatureRule = quadRuleRef[0];
        clusterRule = QuadCubatureRule.degree1BoxRule();
    }

    /** Jacobi iterations, serial or parallel: both gather with the radiance of the previous iteration */
    public boolean isJacobi() {
        return galerkinIterationMethod == GalerkinIterationMethod.JACOBI
            || galerkinIterationMethod == GalerkinIterationMethod.PARALLEL_JACOBI;
    }

    public RadianceElementTable getRadianceElements() {
        return radianceElements;
    }

    public void setRadianceElements(RadianceElementTable inRadianceElements) {
        radianceElements = inRadianceElements;
    }

    /**
    Top-level element of a face, without checks (hot path); see
    GalerkinElement.fromPatch for the checked lookup
    */
    public GalerkinElement topLevelElement(Patch patch) {
        return (GalerkinElement)radianceElements.get(patch.getFaceIndex());
    }

    /**
    Element bound to a face, of any radiance method, or null when no method is
    bound or the face has no element (checked lookup for tools)
    */
    public Element boundElement(Patch patch) {
        if ( radianceElements == null || patch == null ) {
            return null;
        }
        return radianceElements.get(patch.getFaceIndex());
    }

    // The scene's table of top-level elements, bound when the method is initialized
    private RadianceElementTable radianceElements = null;
}
