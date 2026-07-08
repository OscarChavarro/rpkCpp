package vsdk.toolkit.galerkin.processing;

import java.util.ArrayList;
import java.util.HashSet;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.memoryManagement.MemoryPool;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.galerkin.GalerkinBasis;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinClusteringStrategy;
import vsdk.toolkit.galerkin.GalerkinErrorNorm;
import vsdk.toolkit.galerkin.GalerkinIterationMethod;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.galerkin.Interaction;
import vsdk.toolkit.galerkin.Shaft;
import vsdk.toolkit.galerkin.ShaftCullStrategy;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.scene.Polygon;
import vsdk.toolkit.environment.geometry.elements.ElementFlags;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.environment.geometry.elements.Patch;

/**
Shaft culling stuff for hierarchical refinement
*/
public class HierarchicalRefinementStrategy {
    private static final int K_SIZE = GalerkinBasis.MAX_BASIS_SIZE * GalerkinBasis.MAX_BASIS_SIZE;
    private static final MemoryPool<float[]> hierarchicalCoefficientsPool =
        new MemoryPool<>(() -> new float[K_SIZE]);
    private static boolean hierarchicalCoefficientsPoolInitialized = false;

    private static void ensureHierarchicalCoefficientsPool() {
        if (!hierarchicalCoefficientsPoolInitialized) {
            hierarchicalCoefficientsPool.init(16L * 1024L * 1024L);
            hierarchicalCoefficientsPoolInitialized = true;
        }
    }

    private static void prepareRefinementIterationState(GalerkinState galerkinState) {
        Statistics statistics = Statistics.instance();
        galerkinState.refinementMinimumArea =
            statistics.radiance.totalArea * galerkinState.relMinElemArea;
        galerkinState.refinementRadianceErrorThreshold =
            hierarchicRefinementColorToError(statistics.radiance.maxSelfEmittedRadiance)
            * galerkinState.relLinkErrorThreshold;
        galerkinState.refinementPowerErrorThresholdNumerator =
            hierarchicRefinementColorToError(statistics.radiance.maxSelfEmittedPower)
            * galerkinState.relLinkErrorThreshold;
        galerkinState.refinementMaxDirectPotential =
            statistics.potential.maxDirectPotential;
    }

    private static final ThreadLocal<Integer> refineTreeDepth = ThreadLocal.withInitial(() -> 0);
    private static void hierarchicRefinementCull(
        Scene scene,
        ArrayList<Geometry>[] candidatesList,
        Interaction interaction,
        boolean isClusteredGeometry,
        GalerkinState galerkinState)
    {
        if ( candidatesList[0] == null ) {
            return;
        }

        if ( galerkinState.shaftCullMode == vsdk.toolkit.galerkin.GalerkinShaftCullMode.DO_SHAFT_CULLING_FOR_REFINEMENT
            || galerkinState.shaftCullMode == vsdk.toolkit.galerkin.GalerkinShaftCullMode.ALWAYS_DO_SHAFT_CULLING ) {
            Shaft shaft = new Shaft();
            Polygon rcvPolygon = new Polygon();
            Polygon srcPolygon = new Polygon();
            BoundingBox srcBounds = new BoundingBox();
            BoundingBox rcvBounds = new BoundingBox();

            if ( galerkinState.exactVisibility != 0
                && !interaction.receiverElement.isCluster()
                && !interaction.sourceElement.isCluster() ) {
                interaction.receiverElement.initPolygon(rcvPolygon);
                interaction.sourceElement.initPolygon(srcPolygon);
                shaft.constructFromPolygonToPolygon(rcvPolygon, srcPolygon);
            }
            else {
                shaft.constructFromBoundingBoxes(
                    interaction.receiverElement.bounds(rcvBounds),
                    interaction.sourceElement.bounds(srcBounds));
            }

            if ( interaction.receiverElement.isCluster() ) {
                shaft.setShaftDontOpen(interaction.receiverElement.geometry);
            }
            else {
                shaft.setShaftOmit(interaction.receiverElement.patch);
            }

            if ( interaction.sourceElement.isCluster() ) {
                shaft.setShaftDontOpen(interaction.sourceElement.geometry);
            }
            else {
                shaft.setShaftOmit(interaction.sourceElement.patch);
            }

            ArrayList<Geometry> arr = new ArrayList<>();
            if ( isClusteredGeometry ) {
                shaft.cullGeometry(scene.clusteredRootGeometry, arr, galerkinState.shaftCullStrategy);
            }
            else {
                shaft.doCulling(candidatesList[0], arr, galerkinState.shaftCullStrategy);
            }
            candidatesList[0] = arr;
        }
    }

    private static void hierarchicRefinementUnCull(
        ArrayList<Geometry>[] candidatesList,
        GalerkinState galerkinState)
    {
        if ( galerkinState.shaftCullMode == vsdk.toolkit.galerkin.GalerkinShaftCullMode.DO_SHAFT_CULLING_FOR_REFINEMENT
            || galerkinState.shaftCullMode == vsdk.toolkit.galerkin.GalerkinShaftCullMode.ALWAYS_DO_SHAFT_CULLING ) {
            Shaft.freeCandidateList(candidatesList[0]);
        }
    }

    private static double hierarchicRefinementColorToError(ColorRgb radiance) {
        return radiance != null ? radiance.maximumComponent() : 0.0;
    }

    private static double hierarchicRefinementLinkErrorThreshold(
        Interaction interaction,
        double receiverArea,
        GalerkinState galerkinState)
    {
        double threshold;
        switch ( galerkinState.errorNorm ) {
            case RADIANCE_ERROR:
                threshold = galerkinState.refinementRadianceErrorThreshold;
                break;
            case POWER_ERROR:
                threshold = galerkinState.refinementPowerErrorThresholdNumerator / (Math.PI * receiverArea);
                break;
            default:
                threshold = galerkinState.relLinkErrorThreshold;
                break;
        }

        if ( galerkinState.importanceDriven != 0
            && (galerkinState.galerkinIterationMethod == GalerkinIterationMethod.JACOBI
                || galerkinState.galerkinIterationMethod == GalerkinIterationMethod.GAUSS_SEIDEL) ) {
            double maxDirect = galerkinState.refinementMaxDirectPotential;
            if ( maxDirect > Numeric.EPSILON ) {
                threshold /= 2.0 * interaction.receiverElement.potential / maxDirect;
            }
        }
        return threshold;
    }

    private static double hierarchicRefinementApproximationError(
        Interaction interaction,
        ColorRgb srcRho,
        ColorRgb rcvRho,
        GalerkinState galerkinState)
    {
        ColorRgb error = new ColorRgb();
        ColorRgb srcRad;

        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.GAUSS_SEIDEL
            || galerkinState.galerkinIterationMethod == GalerkinIterationMethod.JACOBI ) {
            if ( interaction.sourceElement.isCluster()
                && interaction.sourceElement != interaction.receiverElement ) {
                srcRad = ClusterTraversalStrategy.maxRadiance(interaction.sourceElement, galerkinState);
            }
            else {
                srcRad = interaction.sourceElement.radiance[0];
            }
        }
        else {
            if ( interaction.sourceElement.isCluster()
                && interaction.sourceElement != interaction.receiverElement ) {
                srcRad = ClusterTraversalStrategy.sourceClusterRadiance(interaction, galerkinState);
            }
            else {
                srcRad = interaction.sourceElement.unShotRadiance[0];
            }
        }

        error.scalarProductScaled(rcvRho, interaction.deltaK, srcRad);
        error.abs();
        return hierarchicRefinementColorToError(error);
    }

    private static double sourceClusterRadianceVariationError(
        Interaction interaction,
        ColorRgb rcvRho,
        double receiverArea,
        GalerkinState galerkinState)
    {
        double K = interaction.K[0];
        if ( K == 0.0 || rcvRho.isBlack() || interaction.sourceElement.radiance[0].isBlack() ) {
            return 0.0;
        }

        Vector3D[] rcVertices = new Vector3D[] {
            new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D(),
            new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D()
        };
        int numberOfReceiverVertices = interaction.receiverElement.vertices(rcVertices);

        ColorRgb minimumSourceRadiance = new ColorRgb();
        ColorRgb maximumSourceRadiance = new ColorRgb();
        ColorRgb error = new ColorRgb();
        minimumSourceRadiance.setMonochrome(Numeric.HUGE_FLOAT_VALUE);
        maximumSourceRadiance.setMonochrome(-Numeric.HUGE_FLOAT_VALUE);
        for ( int i = 0; i < numberOfReceiverVertices; i++ ) {
            ColorRgb radiance = ClusterTraversalStrategy.clusterRadianceToSamplePoint(
                interaction.sourceElement, rcVertices[i], galerkinState);
            minimumSourceRadiance.minimum(minimumSourceRadiance, radiance);
            maximumSourceRadiance.maximum(maximumSourceRadiance, radiance);
        }
        error.subtract(maximumSourceRadiance, minimumSourceRadiance);
        error.scalarProductScaled(rcvRho, (float)(K / receiverArea), error);
        error.abs();
        return hierarchicRefinementColorToError(error);
    }

    private static InteractionEvaluationCode hierarchicRefinementEvaluateInteraction(
        Interaction interaction,
        GalerkinState galerkinState)
    {
        if ( !galerkinState.hierarchical ) {
            return InteractionEvaluationCode.ACCURATE_ENOUGH;
        }

        double receiveArea;
        ColorRgb rcvRho = new ColorRgb();
        if ( interaction.receiverElement.isCluster() ) {
            rcvRho.setMonochrome(1.0f);
            receiveArea = ClusterTraversalStrategy.receiverArea(interaction, galerkinState);
        }
        else {
            rcvRho = interaction.receiverElement.patch.radianceData.Rd;
            receiveArea = interaction.receiverElement.area;
        }

        ColorRgb srcRho = new ColorRgb();
        if ( interaction.sourceElement.isCluster() ) {
            srcRho.setMonochrome(1.0f);
        }
        else {
            srcRho = interaction.sourceElement.patch.radianceData.Rd;
        }

        double threshold = hierarchicRefinementLinkErrorThreshold(interaction, receiveArea, galerkinState);
        double error = hierarchicRefinementApproximationError(interaction, srcRho, rcvRho, galerkinState);

        if ( interaction.sourceElement.isCluster()
            && error < threshold
            && galerkinState.clusteringStrategy != GalerkinClusteringStrategy.ISOTROPIC ) {
            error += sourceClusterRadianceVariationError(interaction, rcvRho, receiveArea, galerkinState);
        }

        double minimumArea = galerkinState.refinementMinimumArea;

        if ( error <= threshold ) {
            return InteractionEvaluationCode.ACCURATE_ENOUGH;
        }

        if ( (!(interaction.sourceElement.isCluster()
                && (interaction.sourceElement.flags & ElementFlags.IS_LIGHT_SOURCE_MASK) != 0))
            && (receiveArea > interaction.sourceElement.area) ) {
            if ( receiveArea > minimumArea ) {
                if ( interaction.receiverElement.isCluster() ) {
                    return InteractionEvaluationCode.SUBDIVIDE_RECEIVER_CLUSTER;
                }
                return InteractionEvaluationCode.REGULAR_SUBDIVIDE_RECEIVER;
            }
        }
        else {
            if ( interaction.sourceElement.isCluster() ) {
                return InteractionEvaluationCode.SUBDIVIDE_SOURCE_CLUSTER;
            }
            else if ( interaction.sourceElement.area > minimumArea ) {
                return InteractionEvaluationCode.REGULAR_SUBDIVIDE_SOURCE;
            }
        }

        return InteractionEvaluationCode.ACCURATE_ENOUGH;
    }

    private static void hierarchicRefinementComputeLightTransport(
        Interaction interaction,
        GalerkinState galerkinState)
    {
        int a = Math.min(interaction.numberOfBasisFunctionsOnReceiver, interaction.receiverElement.basisSize);
        int b = Math.min(interaction.numberOfBasisFunctionsOnSource, interaction.sourceElement.basisSize);
        if ( a > interaction.receiverElement.basisUsed ) {
            interaction.receiverElement.basisUsed = (byte)a;
        }
        if ( b > interaction.sourceElement.basisUsed ) {
            interaction.sourceElement.basisUsed = (byte)b;
        }

        ColorRgb[] srcRad;
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.SOUTH_WELL ) {
            srcRad = interaction.sourceElement.unShotRadiance;
        }
        else {
            srcRad = interaction.sourceElement.radiance;
        }

        ColorRgb linkClusterRad = new ColorRgb();
        if ( interaction.sourceElement.isCluster() && interaction.sourceElement != interaction.receiverElement ) {
            linkClusterRad = ClusterTraversalStrategy.sourceClusterRadiance(interaction, galerkinState);
            srcRad = new ColorRgb[] {linkClusterRad};
        }

        if ( interaction.receiverElement.isCluster() && interaction.sourceElement != interaction.receiverElement ) {
            ClusterTraversalStrategy.gatherRadiance(interaction, srcRad, galerkinState);
        }
        else {
            ColorRgb[] rcvRad = interaction.receiverElement.receivedRadiance;
            if ( interaction.numberOfBasisFunctionsOnReceiver == 1
                && interaction.numberOfBasisFunctionsOnSource == 1 ) {
                rcvRad[0].addScaled(rcvRad[0], interaction.K[0], srcRad[0]);
            }
            else {
                for ( int alpha = 0; alpha < a; alpha++ ) {
                    for ( int beta = 0; beta < b; beta++ ) {
                        rcvRad[alpha].addScaled(
                            rcvRad[alpha],
                            interaction.K[alpha * interaction.numberOfBasisFunctionsOnSource + beta],
                            srcRad[beta]);
                    }
                }
            }
        }

        if ( galerkinState.importanceDriven != 0 ) {
            float K = interaction.K[0];
            ColorRgb rcvRho = new ColorRgb();
            ColorRgb srcRho = new ColorRgb();

            if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.GAUSS_SEIDEL
                 || galerkinState.galerkinIterationMethod == GalerkinIterationMethod.JACOBI ) {
                if ( interaction.receiverElement.isCluster() ) {
                    rcvRho.setMonochrome(1.0f);
                }
                else {
                    rcvRho = interaction.receiverElement.patch.radianceData.Rd;
                }
                interaction.sourceElement.receivedPotential +=
                    K * hierarchicRefinementColorToError(rcvRho) * interaction.receiverElement.potential;
            }
            else if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.SOUTH_WELL ) {
                if ( interaction.sourceElement.isCluster() ) {
                    srcRho.setMonochrome(1.0f);
                }
                else {
                    srcRho = interaction.sourceElement.patch.radianceData.Rd;
                }
                interaction.receiverElement.receivedPotential +=
                    K * hierarchicRefinementColorToError(srcRho) * interaction.sourceElement.unShotPotential;
            }
        }
    }

    private static int hierarchicRefinementCreateSubdivisionLink(
        Scene scene,
        ArrayList<Geometry> candidatesList,
        GalerkinElement receiverElement,
        GalerkinElement sourceElement,
        Interaction interaction,
        GalerkinState galerkinState)
    {
        interaction.receiverElement = receiverElement;
        interaction.sourceElement = sourceElement;

        if ( interaction.receiverElement.isCluster() ) {
            interaction.numberOfBasisFunctionsOnReceiver = 1;
        } else {
            interaction.numberOfBasisFunctionsOnReceiver = receiverElement.basisSize;
        }

        if ( interaction.sourceElement.isCluster() ) {
            interaction.numberOfBasisFunctionsOnSource = 1;
        } else {
            interaction.numberOfBasisFunctionsOnSource = sourceElement.basisSize;
        }

        boolean isSceneGeometry = (candidatesList == scene.geometryList);
        boolean isClusteredGeometry = (candidatesList == scene.clusteredGeometryList);
        FormFactorStrategy.computeAreaToAreaFormFactorVisibility(
            scene.voxelGrid,
            candidatesList,
            isSceneGeometry,
            isClusteredGeometry,
            interaction,
            galerkinState);
        return interaction.visibility != 0 ? 1 : 0;
    }

    private static void hierarchicRefinementStoreInteraction(Interaction interaction, GalerkinState galerkinState) {
        Interaction newInteraction = Interaction.interactionDuplicate(interaction);
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.SOUTH_WELL ) {
            if ( interaction.sourceElement.interactions == null ) {
                interaction.sourceElement.interactions = new ArrayList<>();
            }
            interaction.sourceElement.interactions.add(newInteraction);
        } else {
            if ( interaction.receiverElement.interactions == null ) {
                interaction.receiverElement.interactions = new ArrayList<>();
            }
            interaction.receiverElement.interactions.add(newInteraction);
        }
    }

    private static void hierarchicRefinementRegularSubdivideSource(
        Scene scene,
        ArrayList<Geometry>[] candidatesList,
        Interaction interaction,
        boolean isClusteredGeometry,
        GalerkinState galerkinState)
    {
        ArrayList<Geometry> backup = candidatesList[0];
        hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
        GalerkinElement sourceElement = interaction.sourceElement;
        GalerkinElement receiverElement = interaction.receiverElement;

        sourceElement.regularSubDivide();
        if ( sourceElement.regularSubElements == null ) {
            hierarchicRefinementComputeLightTransport(interaction, galerkinState);
            return;
        }

        for ( int i = 0; i < 4; i++ ) {
            if ( !(sourceElement.regularSubElements[i] instanceof GalerkinElement) ) {
                continue;
            }
            GalerkinElement child = (GalerkinElement)sourceElement.regularSubElements[i];
            Interaction subInteraction = new Interaction();
            ensureHierarchicalCoefficientsPool();
            boolean usingPool = true;
            subInteraction.K = hierarchicalCoefficientsPool.allocate(K_SIZE);
            if (subInteraction.K == null) {
                if (hierarchicalCoefficientsPool.expand(K_SIZE * 128)) {
                    subInteraction.K = hierarchicalCoefficientsPool.allocate(K_SIZE);
                }
                if (subInteraction.K == null) {
                    usingPool = false;
                    subInteraction.K = new float[K_SIZE];
                }
            }
            subInteraction.ownsK = !usingPool;
            if ( hierarchicRefinementCreateSubdivisionLink(
                    scene,
                    candidatesList[0],
                    receiverElement,
                    child,
                    subInteraction,
                    galerkinState) != 0
                && !refineRecursive(scene, candidatesList, subInteraction, galerkinState) ) {
                hierarchicRefinementStoreInteraction(subInteraction, galerkinState);
            }
            if (usingPool) {
                subInteraction.K = null;
                hierarchicalCoefficientsPool.free(K_SIZE);
            }
        }
        hierarchicRefinementUnCull(candidatesList, galerkinState);
        candidatesList[0] = backup;
    }

    private static void hierarchicRefinementRegularSubdivideReceiver(
        Scene scene,
        ArrayList<Geometry>[] candidatesList,
        Interaction interaction,
        boolean isClusteredGeometry,
        GalerkinState galerkinState)
    {
        ArrayList<Geometry> backup = candidatesList[0];
        hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
        GalerkinElement sourceElement = interaction.sourceElement;
        GalerkinElement receiverElement = interaction.receiverElement;

        receiverElement.regularSubDivide();
        if ( receiverElement.regularSubElements == null ) {
            hierarchicRefinementComputeLightTransport(interaction, galerkinState);
            return;
        }

        for ( int i = 0; i < 4; i++ ) {
            if ( !(receiverElement.regularSubElements[i] instanceof GalerkinElement) ) {
                continue;
            }
            GalerkinElement child = (GalerkinElement)receiverElement.regularSubElements[i];
            Interaction subInteraction = new Interaction();
            ensureHierarchicalCoefficientsPool();
            boolean usingPool = true;
            subInteraction.K = hierarchicalCoefficientsPool.allocate(K_SIZE);
            if (subInteraction.K == null) {
                if (hierarchicalCoefficientsPool.expand(K_SIZE * 128)) {
                    subInteraction.K = hierarchicalCoefficientsPool.allocate(K_SIZE);
                }
                if (subInteraction.K == null) {
                    usingPool = false;
                    subInteraction.K = new float[K_SIZE];
                }
            }
            subInteraction.ownsK = !usingPool;
            if ( hierarchicRefinementCreateSubdivisionLink(
                    scene,
                    candidatesList[0],
                    child,
                    sourceElement,
                    subInteraction,
                    galerkinState) != 0
                && !refineRecursive(scene, candidatesList, subInteraction, galerkinState) ) {
                hierarchicRefinementStoreInteraction(subInteraction, galerkinState);
            }
            if (usingPool) {
                subInteraction.K = null;
                hierarchicalCoefficientsPool.free(K_SIZE);
            }
        }
        hierarchicRefinementUnCull(candidatesList, galerkinState);
        candidatesList[0] = backup;
    }

    private static void hierarchicRefinementSubdivideSourceCluster(
        Scene scene,
        ArrayList<Geometry>[] candidatesList,
        Interaction interaction,
        boolean isClusteredGeometry,
        GalerkinState galerkinState)
    {
        ArrayList<Geometry> backup = candidatesList[0];
        hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
        GalerkinElement sourceElement = interaction.sourceElement;
        GalerkinElement receiverElement = interaction.receiverElement;

        for ( int i = 0;
              sourceElement.irregularSubElements != null && i < sourceElement.irregularSubElements.size();
              i++ ) {
            if ( !(sourceElement.irregularSubElements.get(i) instanceof GalerkinElement) ) {
                continue;
            }
            GalerkinElement childElement = (GalerkinElement)sourceElement.irregularSubElements.get(i);
            if ( !childElement.isCluster() ) {
                Patch patch = childElement.patch;
                if ( patch != null && (
                    (receiverElement.isCluster() && receiverElement.geometry != null
                        && receiverElement.geometry.getBoundingBox().behindPlane(patch.normal, patch.planeConstant))
                    || (!receiverElement.isCluster() && receiverElement.patch != null
                        && !receiverElement.patch.facing(patch)) ) ) {
                    continue;
                }
            }
            Interaction subInteraction = new Interaction();
            ensureHierarchicalCoefficientsPool();
            boolean usingPool = true;
            subInteraction.K = hierarchicalCoefficientsPool.allocate(K_SIZE);
            if (subInteraction.K == null) {
                if (hierarchicalCoefficientsPool.expand(K_SIZE * 128)) {
                    subInteraction.K = hierarchicalCoefficientsPool.allocate(K_SIZE);
                }
                if (subInteraction.K == null) {
                    usingPool = false;
                    subInteraction.K = new float[K_SIZE];
                }
            }
            subInteraction.ownsK = !usingPool;

            if ( hierarchicRefinementCreateSubdivisionLink(
                    scene,
                    candidatesList[0],
                    receiverElement,
                    childElement,
                    subInteraction,
                    galerkinState) != 0
                && !refineRecursive(scene, candidatesList, subInteraction, galerkinState) ) {
                hierarchicRefinementStoreInteraction(subInteraction, galerkinState);
            }
            if (usingPool) {
                subInteraction.K = null;
                hierarchicalCoefficientsPool.free(K_SIZE);
            }
        }
        hierarchicRefinementUnCull(candidatesList, galerkinState);
        candidatesList[0] = backup;
    }

    private static void hierarchicRefinementSubdivideReceiverCluster(
        Scene scene,
        ArrayList<Geometry>[] candidatesList,
        Interaction interaction,
        boolean isClusteredGeometry,
        GalerkinState galerkinState)
    {
        ArrayList<Geometry> backup = candidatesList[0];
        hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
        GalerkinElement sourceElement = interaction.sourceElement;
        GalerkinElement receiverElement = interaction.receiverElement;

        for ( int i = 0;
              receiverElement.irregularSubElements != null && i < receiverElement.irregularSubElements.size();
              i++ ) {
            if ( !(receiverElement.irregularSubElements.get(i) instanceof GalerkinElement) ) {
                continue;
            }
            GalerkinElement child = (GalerkinElement)receiverElement.irregularSubElements.get(i);
            if ( !child.isCluster() ) {
                Patch patch = child.patch;
                if ( patch != null && (
                    (sourceElement.isCluster() && sourceElement.geometry != null
                        && sourceElement.geometry.getBoundingBox().behindPlane(patch.normal, patch.planeConstant))
                    || (!sourceElement.isCluster() && sourceElement.patch != null
                        && !sourceElement.patch.facing(patch)) ) ) {
                    continue;
                }
            }
            Interaction subInteraction = new Interaction();
            ensureHierarchicalCoefficientsPool();
            boolean usingPool = true;
            subInteraction.K = hierarchicalCoefficientsPool.allocate(K_SIZE);
            if (subInteraction.K == null) {
                if (hierarchicalCoefficientsPool.expand(K_SIZE * 128)) {
                    subInteraction.K = hierarchicalCoefficientsPool.allocate(K_SIZE);
                }
                if (subInteraction.K == null) {
                    usingPool = false;
                    subInteraction.K = new float[K_SIZE];
                }
            }
            subInteraction.ownsK = !usingPool;
            if ( hierarchicRefinementCreateSubdivisionLink(
                    scene,
                    candidatesList[0],
                    child,
                    sourceElement,
                    subInteraction,
                    galerkinState) != 0
                && !refineRecursive(scene, candidatesList, subInteraction, galerkinState) ) {
                hierarchicRefinementStoreInteraction(subInteraction, galerkinState);
            }
            if (usingPool) {
                subInteraction.K = null;
                hierarchicalCoefficientsPool.free(K_SIZE);
            }
        }
        hierarchicRefinementUnCull(candidatesList, galerkinState);
        candidatesList[0] = backup;
    }

    private static boolean refineRecursive(
        Scene scene,
        ArrayList<Geometry>[] candidatesList,
        Interaction interaction,
        GalerkinState galerkinState)
    {
        boolean refined;

        boolean isClusteredGeometry = (candidatesList[0] == scene.clusteredGeometryList);
        switch ( hierarchicRefinementEvaluateInteraction(interaction, galerkinState) ) {
            case ACCURATE_ENOUGH:
                hierarchicRefinementComputeLightTransport(interaction, galerkinState);
                refined = false;
                break;
            case REGULAR_SUBDIVIDE_SOURCE:
                hierarchicRefinementRegularSubdivideSource(
                    scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
                refined = true;
                break;
            case REGULAR_SUBDIVIDE_RECEIVER:
                hierarchicRefinementRegularSubdivideReceiver(
                    scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
                refined = true;
                break;
            case SUBDIVIDE_SOURCE_CLUSTER:
                hierarchicRefinementSubdivideSourceCluster(
                    scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
                refined = true;
                break;
            case SUBDIVIDE_RECEIVER_CLUSTER:
                hierarchicRefinementSubdivideReceiverCluster(
                    scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
                refined = true;
                break;
            default:
                refined = false;
                break;
        }

        return refined;
    }

    private static boolean refineInteraction(Scene scene, Interaction interaction, GalerkinState galerkinState) {
        ArrayList<Geometry> candidateOccluderList = scene.clusteredGeometryList;

        if ( galerkinState.exactVisibility != 0 && interaction.visibility == (byte)255 ) {
            candidateOccluderList = null;
        }

        @SuppressWarnings("unchecked")
        ArrayList<Geometry>[] arr = new ArrayList[] {candidateOccluderList};
        return refineRecursive(scene, arr, interaction, galerkinState);
    }

    public static void refineInteractions(
        Scene scene,
        GalerkinElement parentElement,
        GalerkinState galerkinState)
    {
        prepareRefinementIterationState(galerkinState);
        HashSet<Integer> activePath = new HashSet<>();
        refineInteractionsInternal(scene, parentElement, galerkinState, activePath);
    }

    private static void refineInteractionsInternal(
        Scene scene,
        GalerkinElement parentElement,
        GalerkinState galerkinState,
        HashSet<Integer> activePath)
    {
        if ( parentElement == null ) {
            return;
        }

        int treeDepth = refineTreeDepth.get();

        if ( activePath.contains(parentElement.id) ) {
            return;
        }

        activePath.add(parentElement.id);
        refineTreeDepth.set(treeDepth + 1);
        try {
            for ( int i = 0;
                  parentElement.irregularSubElements != null && i < parentElement.irregularSubElements.size();
                  i++ ) {
                if ( parentElement.irregularSubElements.get(i) instanceof GalerkinElement ) {
                    refineInteractionsInternal(
                        scene,
                        (GalerkinElement)parentElement.irregularSubElements.get(i),
                        galerkinState,
                        activePath);
                }
            }

            if ( parentElement.regularSubElements != null ) {
                for ( int i = 0; i < 4; i++ ) {
                    if ( parentElement.regularSubElements[i] instanceof GalerkinElement ) {
                        refineInteractionsInternal(
                            scene,
                            (GalerkinElement)parentElement.regularSubElements[i],
                            galerkinState,
                            activePath);
                    }
                }
            }

            // Refined interactions are removed with order-preserving in-place compaction
            // to avoid one temporary list plus O(n) remove(Object) scans per element
            ArrayList<Object> interactions = parentElement.interactions;
            if ( interactions != null ) {
                int writeIndex = 0;
                for ( int readIndex = 0; readIndex < interactions.size(); readIndex++ ) {
                    Object o = interactions.get(readIndex);
                    if ( !(o instanceof Interaction) ) {
                        if ( writeIndex != readIndex ) {
                            interactions.set(writeIndex, o);
                        }
                        writeIndex++;
                        continue;
                    }
                    Interaction interaction = (Interaction)o;
                    if ( refineInteraction(scene, interaction, galerkinState) ) {
                        Interaction.interactionDestroy(interaction);
                    } else {
                        if ( writeIndex != readIndex ) {
                            interactions.set(writeIndex, interaction);
                        }
                        writeIndex++;
                    }
                }
                if ( writeIndex < interactions.size() ) {
                    interactions.subList(writeIndex, interactions.size()).clear();
                }
            }
        }
        finally {
            activePath.remove(parentElement.id);
            refineTreeDepth.set(treeDepth);
        }
    }
}
