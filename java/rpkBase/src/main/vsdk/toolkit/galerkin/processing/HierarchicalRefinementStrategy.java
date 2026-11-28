package vsdk.toolkit.galerkin.processing;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicReference;
import java.util.concurrent.locks.ReentrantLock;
import java.util.function.IntConsumer;
import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.memoryManagement.MemoryPool;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.statistics.ShadowStatistics;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.galerkin.GalerkinBasis;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinClusteringStrategy;
import vsdk.toolkit.galerkin.GalerkinErrorNorm;
import vsdk.toolkit.galerkin.GalerkinIterationMethod;
import vsdk.toolkit.galerkin.GalerkinShaftCullMode;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.galerkin.Interaction;
import vsdk.toolkit.galerkin.Shaft;
import vsdk.toolkit.galerkin.ShaftCullStrategy;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.scene.SceneQuery;
import vsdk.toolkit.scene.SceneQueryWorkspace;
import vsdk.toolkit.scene.ConvexPolygon;
import vsdk.toolkit.environment.geometry.elements.Element;
import vsdk.toolkit.environment.geometry.elements.ElementFlags;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;
import vsdk.toolkit.skin.RpkGeometry;
import vsdk.toolkit.environment.geometry.elements.Patch;

/**
Shaft culling stuff for hierarchical refinement
*/
public class HierarchicalRefinementStrategy {
    private static final int K_SIZE = GalerkinBasis.MAX_BASIS_SIZE * GalerkinBasis.MAX_BASIS_SIZE;
    private static final MemoryPool<double[]> hierarchicalCoefficientsPool =
        new MemoryPool<>(() -> new double[K_SIZE]);
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
        ArrayList<RpkGeometry>[] candidatesList,
        Interaction interaction,
        boolean isClusteredGeometry,
        GalerkinState galerkinState)
    {
        if ( candidatesList[0] == null ) {
            return;
        }

        if ( galerkinState.shaftCullMode == vsdk.toolkit.galerkin.GalerkinShaftCullMode.DO_SHAFT_CULLING_FOR_REFINEMENT
            || galerkinState.shaftCullMode == vsdk.toolkit.galerkin.GalerkinShaftCullMode.ALWAYS_DO_SHAFT_CULLING ) {
            Shaft shaft = new Shaft(scene.getFaceBounds());
            ConvexPolygon rcvPolygon = new ConvexPolygon();
            ConvexPolygon srcPolygon = new ConvexPolygon();
            RpkAxisAlignedBoundingBox srcBounds = new RpkAxisAlignedBoundingBox();
            RpkAxisAlignedBoundingBox rcvBounds = new RpkAxisAlignedBoundingBox();

            if ( galerkinState.exactVisibility != 0
                && !interaction.receiverElement.isCluster()
                && !interaction.sourceElement.isCluster() ) {
                interaction.receiverElement.initPolygon(rcvPolygon);
                interaction.sourceElement.initPolygon(srcPolygon);
                shaft.constructFromPolygonToPolygon(rcvPolygon, srcPolygon);
            }
            else {
                shaft.constructFromBoundingBoxes( interaction.receiverElement.bounds(rcvBounds), interaction.sourceElement.bounds(srcBounds));
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

            ArrayList<RpkGeometry> arr = new ArrayList<>();
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
        ArrayList<RpkGeometry>[] candidatesList,
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
            && (galerkinState.isJacobi()
                || galerkinState.galerkinIterationMethod == GalerkinIterationMethod.GAUSS_SEIDEL) ) {
            double maxDirect = galerkinState.refinementMaxDirectPotential;
            if ( maxDirect > VSDK.EPSILON ) {
                threshold /= 2.0 * interaction.receiverElement.potential / maxDirect;
            }
        }
        return threshold;
    }

    private static double hierarchicRefinementApproximationError(
        Interaction interaction,
        ColorRgb srcRho,
        ColorRgb rcvRho,
        ScratchVisibilityWorkspace scratch,
        GalerkinState galerkinState)
    {
        ColorRgb error = new ColorRgb();
        ColorRgb srcRad;

        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.GAUSS_SEIDEL
            || galerkinState.isJacobi() ) {
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
                srcRad = ClusterTraversalStrategy.sourceClusterRadiance(interaction, scratch, galerkinState);
            }
            else {
                srcRad = interaction.sourceElement.unShotRadiance[0];
            }
        }

        error = rcvRho.multiply(interaction.deltaK).multiply(srcRad);
        error = error.absolute();
        return hierarchicRefinementColorToError(error);
    }

    private static double sourceClusterRadianceVariationError(
        Interaction interaction,
        ColorRgb rcvRho,
        double receiverArea,
        ScratchVisibilityWorkspace scratch,
        GalerkinState galerkinState)
    {
        double K = interaction.K[0];
        if ( K == 0.0 || rcvRho.isBlack() || interaction.sourceElement.radiance[0].isBlack() ) {
            return 0.0;
        }

        Vector3Dd[] rcVertices = new Vector3Dd[] {
            new Vector3Dd(), new Vector3Dd(), new Vector3Dd(), new Vector3Dd(),
            new Vector3Dd(), new Vector3Dd(), new Vector3Dd(), new Vector3Dd()
        };
        int numberOfReceiverVertices = interaction.receiverElement.vertices(rcVertices);

        ColorRgb minimumSourceRadiance = new ColorRgb();
        ColorRgb maximumSourceRadiance = new ColorRgb();
        ColorRgb error = new ColorRgb();
        minimumSourceRadiance = new ColorRgb(Double.MAX_VALUE, Double.MAX_VALUE, Double.MAX_VALUE);
        maximumSourceRadiance = new ColorRgb(-Double.MAX_VALUE, -Double.MAX_VALUE, -Double.MAX_VALUE);
        for ( int i = 0; i < numberOfReceiverVertices; i++ ) {
            ColorRgb radiance = ClusterTraversalStrategy.clusterRadianceToSamplePoint(
                interaction.sourceElement, rcVertices[i], scratch, galerkinState);
            minimumSourceRadiance = minimumSourceRadiance.minimum(radiance);
            maximumSourceRadiance = maximumSourceRadiance.maximum(radiance);
        }
        error = maximumSourceRadiance.subtract(minimumSourceRadiance);
        error = rcvRho.multiply((double)(K / receiverArea)).multiply(error);
        error = error.absolute();
        return hierarchicRefinementColorToError(error);
    }

    private static InteractionEvaluationCode hierarchicRefinementEvaluateInteraction(
        Interaction interaction,
        ScratchVisibilityWorkspace scratch,
        GalerkinState galerkinState)
    {
        if ( !galerkinState.hierarchical ) {
            return InteractionEvaluationCode.ACCURATE_ENOUGH;
        }

        double receiveArea;
        ColorRgb rcvRho = new ColorRgb();
        if ( interaction.receiverElement.isCluster() ) {
            rcvRho = new ColorRgb(1.0, 1.0, 1.0);
            receiveArea = ClusterTraversalStrategy.receiverArea(interaction, scratch, galerkinState);
        }
        else {
            rcvRho = interaction.receiverElement.galerkinState.topLevelElement(interaction.receiverElement.patch).Rd;
            receiveArea = interaction.receiverElement.area;
        }

        ColorRgb srcRho = new ColorRgb();
        if ( interaction.sourceElement.isCluster() ) {
            srcRho = new ColorRgb(1.0, 1.0, 1.0);
        }
        else {
            srcRho = interaction.sourceElement.galerkinState.topLevelElement(interaction.sourceElement.patch).Rd;
        }

        double threshold = hierarchicRefinementLinkErrorThreshold(interaction, receiveArea, galerkinState);
        double error = hierarchicRefinementApproximationError(interaction, srcRho, rcvRho, scratch, galerkinState);

        if ( interaction.sourceElement.isCluster()
            && error < threshold
            && galerkinState.clusteringStrategy != GalerkinClusteringStrategy.ISOTROPIC ) {
            error += sourceClusterRadianceVariationError(interaction, rcvRho, receiveArea, scratch, galerkinState);
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
        ScratchVisibilityWorkspace scratch,
        GalerkinState galerkinState)
    {
        receiverLightTransport(interaction, scratch, null, null, galerkinState);
        sourceLightTransport(interaction, galerkinState);
    }

    /**
    The part of light transport that writes the receiver: the number of
    effectively used radiance coefficients and the received radiance. It reads
    only the source radiance (frozen during gathering). What the plan, when given,
    holds is used instead of computing it (see planClusterTransport)
    */
    private static void receiverLightTransport(
        Interaction interaction,
        ScratchVisibilityWorkspace scratch,
        ClusterTransportPlan plan,
        ArrayList<ArrayList<ElementSamples>> samplePools,
        GalerkinState galerkinState)
    {
        int a = Math.min(interaction.numberOfBasisFunctionsOnReceiver, interaction.receiverElement.basisSize);
        int b = Math.min(interaction.numberOfBasisFunctionsOnSource, interaction.sourceElement.basisSize);
        if ( a > interaction.receiverElement.basisUsed ) {
            interaction.receiverElement.basisUsed = (byte)a;
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
            if ( plan != null && plan.hasSourceRadiance() ) {
                linkClusterRad = plan.getSourceRadiance();
            }
            else {
                linkClusterRad = ClusterTraversalStrategy.sourceClusterRadiance(interaction, scratch, galerkinState);
            }
            srcRad = new ColorRgb[] {linkClusterRad};
        }

        if ( interaction.receiverElement.isCluster() && interaction.sourceElement != interaction.receiverElement ) {
            if ( plan != null && plan.hasReceiverSamples() ) {
                ClusterTraversalStrategy.gatherRadianceFromSamples(
                    interaction, srcRad, plan.getPixelArea(),
                    samplePools.get(plan.getPool()), plan.getFirstSample(), plan.getNumberOfSamples());
            }
            else {
                ClusterTraversalStrategy.gatherRadiance(interaction, srcRad, scratch, galerkinState);
            }
        }
        else {
            ColorRgb[] rcvRad = interaction.receiverElement.receivedRadiance;
            if ( interaction.numberOfBasisFunctionsOnReceiver == 1
                && interaction.numberOfBasisFunctionsOnSource == 1 ) {
                rcvRad[0] = rcvRad[0].addScaled(interaction.K[0], srcRad[0]);
            }
            else {
                for ( int alpha = 0; alpha < a; alpha++ ) {
                    for ( int beta = 0; beta < b; beta++ ) {
                        rcvRad[alpha] = rcvRad[alpha].addScaled(interaction.K[alpha * interaction.numberOfBasisFunctionsOnSource + beta], srcRad[beta]);
                    }
                }
            }
        }

        if ( galerkinState.importanceDriven != 0
             && galerkinState.galerkinIterationMethod == GalerkinIterationMethod.SOUTH_WELL ) {
            double K = interaction.K[0];
            ColorRgb srcRho = new ColorRgb();
            if ( interaction.sourceElement.isCluster() ) {
                srcRho = new ColorRgb(1.0, 1.0, 1.0);
            }
            else {
                srcRho = interaction.sourceElement.galerkinState.topLevelElement(interaction.sourceElement.patch).Rd;
            }
            interaction.receiverElement.receivedPotential +=
                K * hierarchicRefinementColorToError(srcRho) * interaction.sourceElement.unShotPotential;
        }
    }

    /**
    The part of light transport that writes the source: the number of effectively
    used radiance coefficients and, for importance-driven gathering, the received
    potential. It reads only the receiver potential (frozen during gathering)
    */
    private static void sourceLightTransport(
        Interaction interaction,
        GalerkinState galerkinState)
    {
        int b = Math.min(interaction.numberOfBasisFunctionsOnSource, interaction.sourceElement.basisSize);
        if ( b > interaction.sourceElement.basisUsed ) {
            interaction.sourceElement.basisUsed = (byte)b;
        }

        if ( galerkinState.importanceDriven != 0 ) {
            if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.GAUSS_SEIDEL
                 || galerkinState.isJacobi() ) {
                double K = interaction.K[0];
                ColorRgb rcvRho = new ColorRgb();
                if ( interaction.receiverElement.isCluster() ) {
                    rcvRho = new ColorRgb(1.0, 1.0, 1.0);
                }
                else {
                    rcvRho = interaction.receiverElement.galerkinState.topLevelElement(interaction.receiverElement.patch).Rd;
                }
                interaction.sourceElement.receivedPotential +=
                    K * hierarchicRefinementColorToError(rcvRho) * interaction.receiverElement.potential;
            }
            else if ( galerkinState.galerkinIterationMethod != GalerkinIterationMethod.SOUTH_WELL ) {
                Logger.reportMessage(null, Logger.FATAL_ERROR, "hierarchicRefinementComputeLightTransport", "Did you introduce a new iteration method or so??");
            }
        }
    }

    private static int hierarchicRefinementCreateSubdivisionLink(
        Scene scene,
        ArrayList<RpkGeometry> candidatesList,
        GalerkinElement receiverElement,
        GalerkinElement sourceElement,
        Interaction interaction,
        GalerkinState galerkinState,
        RefinementWorkspace workspace)
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
        FormFactorStrategy.computeAreaToAreaFormFactorVisibility( scene.getSceneQuery(), workspace.getSceneQueryWorkspace(), workspace.getFormFactorWorkspace(), candidatesList, isSceneGeometry, isClusteredGeometry, interaction, galerkinState);

        // Warnings kept by a parallel worker become events, in their order
        RefinementRecord record = workspace.getRecord();
        if ( record != null ) {
            ArrayList<GalerkinElement> pairs = record.getNodesTooClose();
            for ( int i = 0; i + 1 < pairs.size(); i += 2 ) {
                record.getEvents().add(RefinementEvent.nodesTooClose(pairs.get(i), pairs.get(i + 1)));
            }
            pairs.clear();
        }

        return interaction.visibility != 0 ? 1 : 0;
    }

    private static void hierarchicRefinementStoreInteraction(
        Interaction interaction,
        GalerkinState galerkinState,
        RefinementWorkspace workspace)
    {
        RefinementRecord record = workspace.getRecord();
        if ( record != null ) {
            // Parallel Jacobi: counted, stored and transported over when committed
            record.getEvents().add(RefinementEvent.newLink(Interaction.uncountedDuplicate(interaction)));
            return;
        }

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

    /**
    Regular subdivision of a surface element. Parallel Jacobi workers take the
    shared lock (subdivision is idempotent, and the children only depend on the
    frozen state of the parent) and record the event: the ids of new children are
    0 until the commit gives them the ids of the serial order
    */
    private static void regularSubDivide(GalerkinElement element, RefinementWorkspace workspace) {
        ReentrantLock lock = workspace.getSubdivisionLock();
        if ( lock == null ) {
            element.regularSubDivide();
            return;
        }

        lock.lock();
        try {
            if ( element.regularSubElements == null ) {
                element.regularSubDivide();
                for ( int i = 0; element.regularSubElements != null && i < 4; i++ ) {
                    element.regularSubElements[i].id = 0;
                }
            }
        }
        finally {
            lock.unlock();
        }
        workspace.getRecord().getEvents().add(RefinementEvent.subdivision(element));
    }

    /**
    Creates the interaction between the receiver and the source (sub-elements of
    a refined interaction), refines it and, if it needs no further refinement,
    stores it. A parallel Jacobi worker may instead leave it to another worker as
    a new task: the FORK event keeps its place in the serial order
    */
    private static void refineOrForkSubInteraction(
        Scene scene,
        ArrayList<RpkGeometry> candidatesList,
        GalerkinElement receiverElement,
        GalerkinElement sourceElement,
        GalerkinState galerkinState,
        RefinementWorkspace workspace)
    {
        RefinementScheduler scheduler = workspace.getScheduler();
        if ( scheduler != null && scheduler.wantsMoreTasks() ) {
            final long task = scheduler.fork(new RefinementTask(receiverElement, sourceElement, candidatesList));
            workspace.getRecord().getEvents().add(RefinementEvent.fork(task));
            return;
        }
        refineSubInteraction(scene, candidatesList, receiverElement, sourceElement, galerkinState, workspace);
    }

    private static void refineSubInteraction(
        Scene scene,
        ArrayList<RpkGeometry> candidatesList,
        GalerkinElement receiverElement,
        GalerkinElement sourceElement,
        GalerkinState galerkinState,
        RefinementWorkspace workspace)
    {
        Interaction subInteraction = new Interaction();
        MemoryPool<double[]> coefficientsPool = workspace.getCoefficientsPool();
        boolean usingPool = true;
        subInteraction.K = coefficientsPool.allocate(K_SIZE);
        if ( subInteraction.K == null ) {
            if ( coefficientsPool.expand(K_SIZE * 128) ) {
                subInteraction.K = coefficientsPool.allocate(K_SIZE);
            }
            if ( subInteraction.K == null ) {
                usingPool = false;
                subInteraction.K = new double[K_SIZE];
            }
        }
        subInteraction.ownsK = !usingPool;

        @SuppressWarnings("unchecked")
        ArrayList<RpkGeometry>[] candidates = new ArrayList[] {candidatesList};
        if ( hierarchicRefinementCreateSubdivisionLink(
                scene,
                candidates[0],
                receiverElement,
                sourceElement,
                subInteraction,
                galerkinState,
                workspace) != 0
            && !refineRecursive(scene, candidates, subInteraction, galerkinState, workspace) ) {
            hierarchicRefinementStoreInteraction(subInteraction, galerkinState, workspace);
        }

        if ( usingPool ) {
            subInteraction.K = null;
            coefficientsPool.free(K_SIZE);
        }
    }

    private static void hierarchicRefinementRegularSubdivideSource(
        Scene scene,
        ArrayList<RpkGeometry>[] candidatesList,
        Interaction interaction,
        boolean isClusteredGeometry,
        GalerkinState galerkinState,
        RefinementWorkspace workspace)
    {
        ArrayList<RpkGeometry> backup = candidatesList[0];
        hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
        GalerkinElement sourceElement = interaction.sourceElement;
        GalerkinElement receiverElement = interaction.receiverElement;

        regularSubDivide(sourceElement, workspace);
        if ( sourceElement.regularSubElements == null ) {
            // Not for surface elements (they have a patch), so never with a record
            if ( workspace.getRecord() == null ) {
                hierarchicRefinementComputeLightTransport(interaction, workspace.getScratchVisibilityWorkspace(), galerkinState);
            }
            return;
        }

        for ( int i = 0; i < 4; i++ ) {
            if ( !(sourceElement.regularSubElements[i] instanceof GalerkinElement) ) {
                continue;
            }
            GalerkinElement child = (GalerkinElement)sourceElement.regularSubElements[i];
            refineOrForkSubInteraction(scene, candidatesList[0], receiverElement, child, galerkinState, workspace);
        }
        hierarchicRefinementUnCull(candidatesList, galerkinState);
        candidatesList[0] = backup;
    }

    private static void hierarchicRefinementRegularSubdivideReceiver(
        Scene scene,
        ArrayList<RpkGeometry>[] candidatesList,
        Interaction interaction,
        boolean isClusteredGeometry,
        GalerkinState galerkinState,
        RefinementWorkspace workspace)
    {
        ArrayList<RpkGeometry> backup = candidatesList[0];
        hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
        GalerkinElement sourceElement = interaction.sourceElement;
        GalerkinElement receiverElement = interaction.receiverElement;

        regularSubDivide(receiverElement, workspace);
        if ( receiverElement.regularSubElements == null ) {
            // Not for surface elements (they have a patch), so never with a record
            if ( workspace.getRecord() == null ) {
                hierarchicRefinementComputeLightTransport(interaction, workspace.getScratchVisibilityWorkspace(), galerkinState);
            }
            return;
        }

        for ( int i = 0; i < 4; i++ ) {
            if ( !(receiverElement.regularSubElements[i] instanceof GalerkinElement) ) {
                continue;
            }
            GalerkinElement child = (GalerkinElement)receiverElement.regularSubElements[i];
            refineOrForkSubInteraction(scene, candidatesList[0], child, sourceElement, galerkinState, workspace);
        }
        hierarchicRefinementUnCull(candidatesList, galerkinState);
        candidatesList[0] = backup;
    }

    private static void hierarchicRefinementSubdivideSourceCluster(
        Scene scene,
        ArrayList<RpkGeometry>[] candidatesList,
        Interaction interaction,
        boolean isClusteredGeometry,
        GalerkinState galerkinState,
        RefinementWorkspace workspace)
    {
        ArrayList<RpkGeometry> backup = candidatesList[0];
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
                        && receiverElement.geometry.getBoundingBox().behindPlane(patch.getNormal(), patch.getPlaneConstant()))
                    || (!receiverElement.isCluster() && receiverElement.patch != null
                        && !receiverElement.patch.facing(patch)) ) ) {
                    continue;
                }
            }
            refineOrForkSubInteraction(scene, candidatesList[0], receiverElement, childElement, galerkinState, workspace);
        }
        hierarchicRefinementUnCull(candidatesList, galerkinState);
        candidatesList[0] = backup;
    }

    private static void hierarchicRefinementSubdivideReceiverCluster(
        Scene scene,
        ArrayList<RpkGeometry>[] candidatesList,
        Interaction interaction,
        boolean isClusteredGeometry,
        GalerkinState galerkinState,
        RefinementWorkspace workspace)
    {
        ArrayList<RpkGeometry> backup = candidatesList[0];
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
                        && sourceElement.geometry.getBoundingBox().behindPlane(patch.getNormal(), patch.getPlaneConstant()))
                    || (!sourceElement.isCluster() && sourceElement.patch != null
                        && !sourceElement.patch.facing(patch)) ) ) {
                    continue;
                }
            }
            refineOrForkSubInteraction(scene, candidatesList[0], child, sourceElement, galerkinState, workspace);
        }
        hierarchicRefinementUnCull(candidatesList, galerkinState);
        candidatesList[0] = backup;
    }

    private static boolean refineRecursive(
        Scene scene,
        ArrayList<RpkGeometry>[] candidatesList,
        Interaction interaction,
        GalerkinState galerkinState,
        RefinementWorkspace workspace)
    {
        boolean refined;

        boolean isClusteredGeometry = (candidatesList[0] == scene.clusteredGeometryList);
        switch ( hierarchicRefinementEvaluateInteraction(interaction, workspace.getScratchVisibilityWorkspace(), galerkinState) ) {
            case ACCURATE_ENOUGH:
                // A parallel Jacobi worker leaves light transport to the commit
                if ( workspace.getRecord() == null ) {
                    hierarchicRefinementComputeLightTransport(interaction, workspace.getScratchVisibilityWorkspace(), galerkinState);
                }
                refined = false;
                break;
            case REGULAR_SUBDIVIDE_SOURCE:
                hierarchicRefinementRegularSubdivideSource(
                    scene, candidatesList, interaction, isClusteredGeometry, galerkinState, workspace);
                refined = true;
                break;
            case REGULAR_SUBDIVIDE_RECEIVER:
                hierarchicRefinementRegularSubdivideReceiver(
                    scene, candidatesList, interaction, isClusteredGeometry, galerkinState, workspace);
                refined = true;
                break;
            case SUBDIVIDE_SOURCE_CLUSTER:
                hierarchicRefinementSubdivideSourceCluster(
                    scene, candidatesList, interaction, isClusteredGeometry, galerkinState, workspace);
                refined = true;
                break;
            case SUBDIVIDE_RECEIVER_CLUSTER:
                hierarchicRefinementSubdivideReceiverCluster(
                    scene, candidatesList, interaction, isClusteredGeometry, galerkinState, workspace);
                refined = true;
                break;
            default:
                refined = false;
                break;
        }

        return refined;
    }

    private static boolean refineInteraction(
        Scene scene,
        Interaction interaction,
        GalerkinState galerkinState,
        RefinementWorkspace workspace)
    {
        ArrayList<RpkGeometry> candidateOccluderList = scene.clusteredGeometryList;

        if ( galerkinState.exactVisibility != 0 && interaction.visibility == (byte)255 ) {
            candidateOccluderList = null;
        }

        @SuppressWarnings("unchecked")
        ArrayList<RpkGeometry>[] arr = new ArrayList[] {candidateOccluderList};
        return refineRecursive(scene, arr, interaction, galerkinState, workspace);
    }

    public static void refineInteractions(
        Scene scene,
        GalerkinElement parentElement,
        GalerkinState galerkinState)
    {
        prepareRefinementIterationState(galerkinState);
        ensureHierarchicalCoefficientsPool();
        RefinementWorkspace workspace = new RefinementWorkspace(
            scene.getSceneQueryWorkspace(),
            galerkinState.formFactorWorkspace,
            galerkinState.scratchVisibilityWorkspace,
            hierarchicalCoefficientsPool,
            null,
            null,
            null);
        HashSet<Integer> activePath = new HashSet<>();
        refineInteractionsInternal(scene, parentElement, galerkinState, activePath, workspace);
    }

    private static void refineInteractionsInternal(
        Scene scene,
        GalerkinElement parentElement,
        GalerkinState galerkinState,
        HashSet<Integer> activePath,
        RefinementWorkspace workspace)
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
                        activePath,
                        workspace);
                }
            }

            if ( parentElement.regularSubElements != null ) {
                for ( int i = 0; i < 4; i++ ) {
                    if ( parentElement.regularSubElements[i] instanceof GalerkinElement ) {
                        refineInteractionsInternal(
                            scene,
                            (GalerkinElement)parentElement.regularSubElements[i],
                            galerkinState,
                            activePath,
                            workspace);
                    }
                }
            }

            // Refined interactions are removed with order-preserving in-place compaction
            // to avoid one temporary list plus O(n) remove(Object) scans per element.
            // Only the interactions present when the loop starts are refined: the links
            // that their refinement stores in this same list (the receiver kept, the
            // source subdivided) already had their light transport when created. The
            // original RenderPark prepends them (InteractionListAdd), out of the
            // iteration's way; this list appends them, so they are kept but not
            // visited, or their light would be transported twice in the iteration
            ArrayList<Object> interactions = parentElement.interactions;
            if ( interactions != null ) {
                final int numberOfInteractions = interactions.size();
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
                    if ( readIndex < numberOfInteractions && refineInteraction(scene, interaction, galerkinState, workspace) ) {
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

    /*
    Parallel Jacobi refinement.
    With Jacobi iterations, refinement only reads state frozen during the
    iteration (radiance, potential, reflectivity, error thresholds), so the
    refinement of each interaction is independent of the others, except for what
    it changes: received radiance and potential (light transport), interaction
    lists (new links), the regular subdivision of elements and their ids, the
    shadow ray counters and the warnings. The workers refine and compute form
    factors with their own scratch and record those changes; the commit then does
    them in the order of the serial refinement (refineInteractionsInternal), so
    that every floating point sum and every list is the serial one.
    Interactions are those of the element lists when the iteration starts, in the
    serial post-order. Neither the serial refinement nor the commit visits the links
    that an element's own refinement appends to its list (receiver kept, source
    subdivided): their light transport was done when they were created.
    Threads are java.lang.Thread (the C++ port uses VITRAL's java::Thread mirror).
    */

    // Java adaptation: the coefficient pools of the workers (MemoryPool<double[]>
    // keeps its arrays) are kept from one iteration to the next instead of being
    // created by every worker in every iteration. Pool w is used only by worker
    // w; the list is changed only before the workers start
    private static final ArrayList<MemoryPool<double[]>> workerCoefficientsPools = new ArrayList<>();

    private static boolean parallelRefinementSupported(GalerkinState galerkinState) {
        return (galerkinState.clusteringStrategy == GalerkinClusteringStrategy.ISOTROPIC
                || galerkinState.clusteringStrategy == GalerkinClusteringStrategy.Z_VISIBILITY)
            && galerkinState.shaftCullMode == GalerkinShaftCullMode.NEVER_DO_SHAFT_CULLING
            && galerkinState.multiResolutionVisibility == 0;
    }

    private static int numberOfRefinementThreads(GalerkinState galerkinState) {
        int threads = galerkinState.numberOfThreads;
        if ( threads <= 0 ) {
            threads = Runtime.getRuntime().availableProcessors();
        }
        return threads < 1 ? 1 : threads;
    }

    /**
    Collects, in the order of refineInteractionsInternal, the elements having
    interactions and their interactions
    */
    private static void collectRefinementItems(
        GalerkinElement parentElement,
        ArrayList<RefinementGroup> groups,
        ArrayList<Interaction> items,
        HashSet<Integer> activePath)
    {
        if ( parentElement == null || activePath.contains(parentElement.id) ) {
            return;
        }

        activePath.add(parentElement.id);
        for ( int i = 0;
              parentElement.irregularSubElements != null && i < parentElement.irregularSubElements.size();
              i++ ) {
            if ( parentElement.irregularSubElements.get(i) instanceof GalerkinElement ) {
                collectRefinementItems(
                    (GalerkinElement)parentElement.irregularSubElements.get(i), groups, items, activePath);
            }
        }

        if ( parentElement.regularSubElements != null ) {
            for ( int i = 0; i < 4; i++ ) {
                if ( parentElement.regularSubElements[i] instanceof GalerkinElement ) {
                    collectRefinementItems(
                        (GalerkinElement)parentElement.regularSubElements[i], groups, items, activePath);
                }
            }
        }

        ArrayList<Object> interactions = parentElement.interactions;
        if ( interactions != null && interactions.size() > 0 ) {
            groups.add(new RefinementGroup(parentElement, items.size(), interactions.size()));
            for ( int i = 0; i < interactions.size(); i++ ) {
                items.add((Interaction)interactions.get(i));
            }
        }
        activePath.remove(parentElement.id);
    }

    /**
    Runs body(w) for w = 0 .. numberOfThreads - 1, each in its own thread, and
    waits for all
    */
    private static void runWorkers(int numberOfThreads, IntConsumer body) {
        final Thread[] threads = new Thread[numberOfThreads];
        final AtomicReference<Throwable> failure = new AtomicReference<>(null);
        for ( int w = 0; w < numberOfThreads; w++ ) {
            final int worker = w;
            threads[w] = new Thread(() -> {
                try {
                    body.accept(worker);
                }
                catch ( Throwable error ) {
                    failure.compareAndSet(null, error);
                }
            });
        }
        for ( int w = 0; w < numberOfThreads; w++ ) {
            threads[w].start();
        }
        for ( int w = 0; w < numberOfThreads; w++ ) {
            boolean joined = false;
            while ( !joined ) {
                try {
                    threads[w].join();
                    joined = true;
                }
                catch ( InterruptedException interruption ) {
                    // Keep waiting: the workers use shared state until they end
                }
            }
        }
        if ( failure.get() != null ) {
            failure.get().printStackTrace();
            Logger.reportMessage(null, Logger.FATAL_ERROR, "runWorkers", "A parallel Jacobi worker failed: " + failure.get());
        }
    }

    /**
    Worker owning the light transport writes of an element. The C++ port hashes
    the element's address; Java hashes its id (unique once the commit is done)
    */
    private static int ownerOf(GalerkinElement element, int numberOfThreads) {
        long x = ((long)element.id & 0xFFFFFFFFL) * 0x9E3779B97F4A7C15L;
        return (int)((x >>> 32) % numberOfThreads);
    }

    /**
    Java adaptation: a growable list of int, as the C++ ArrayList<int> (without
    boxing every index of the light transport partitions)
    */
    private static final class IntList {
        private int[] values = new int[16];
        private int size = 0;

        void add(int value) {
            if ( size == values.length ) {
                values = Arrays.copyOf(values, 2 * size);
            }
            values[size++] = value;
        }

        int size() {
            return size;
        }

        int get(int index) {
            return values[index];
        }
    }

    /**
    Light transport over the interactions, as if done one after the other in
    their order. Each element gets its sums in that order: the interactions are
    partitioned by the element written (receiver, then in a second pass source),
    keeping their order inside each part, and each part goes to one worker.
    With a clustering strategy other than isotropic, light transport to a receiver
    cluster writes the surface elements in it, which other receivers (the clusters
    containing it, or contained in it) write too, so it is done serially, with the
    solver's scratch visibility workspace; with Z-visibility, its renderings are
    made ahead in parallel (applyClusterLightTransports).
    */
    private static void applyLightTransports(
        ArrayList<Interaction> transports,
        GalerkinState galerkinState,
        int numberOfThreads)
    {
        final int n = transports.size();
        if ( numberOfThreads > 1 && galerkinState.clusteringStrategy == GalerkinClusteringStrategy.Z_VISIBILITY ) {
            applyClusterLightTransports(transports, galerkinState, numberOfThreads);
            return;
        }
        if ( numberOfThreads <= 1 || n < 4096 || galerkinState.clusteringStrategy != GalerkinClusteringStrategy.ISOTROPIC ) {
            for ( int i = 0; i < n; i++ ) {
                hierarchicRefinementComputeLightTransport(transports.get(i), galerkinState.scratchVisibilityWorkspace, galerkinState);
            }
            return;
        }

        // parts[slice * W + owner]: indices of the slice's interactions, in order
        final int W = numberOfThreads;
        final IntList[] receiverParts = new IntList[W * W];
        final IntList[] sourceParts = new IntList[W * W];
        for ( int i = 0; i < W * W; i++ ) {
            receiverParts[i] = new IntList();
            sourceParts[i] = new IntList();
        }
        final int sliceSize = (n + W - 1) / W;
        runWorkers(W, slice -> {
            final int first = slice * sliceSize;
            final int end = Math.min(first + sliceSize, n);
            for ( int i = first; i < end; i++ ) {
                Interaction interaction = transports.get(i);
                receiverParts[slice * W + ownerOf(interaction.receiverElement, W)].add(i);
                sourceParts[slice * W + ownerOf(interaction.sourceElement, W)].add(i);
            }
        });
        runWorkers(W, owner -> {
            for ( int slice = 0; slice < W; slice++ ) {
                IntList part = receiverParts[slice * W + owner];
                for ( int k = 0; k < part.size(); k++ ) {
                    receiverLightTransport(transports.get(part.get(k)), null, null, null, galerkinState);
                }
            }
        });
        runWorkers(W, owner -> {
            for ( int slice = 0; slice < W; slice++ ) {
                IntList part = sourceParts[slice * W + owner];
                for ( int k = 0; k < part.size(); k++ ) {
                    sourceLightTransport(transports.get(part.get(k)), galerkinState);
                }
            }
        });
    }

    /**
    Computes ahead, in the plan, what the light transport over the interaction
    takes from the scratch visibility renderer (Z-visibility clustering): the
    radiance of a source cluster and the samples of a receiver cluster seen from
    outside. It reads only state frozen during the iteration.
    Java adaptation: pixelArea is the worker's out-parameter scratch for
    ClusterTraversalStrategy.receiverSamples
    */
    private static void planClusterTransport(
        Interaction interaction,
        ScratchVisibilityWorkspace scratch,
        GalerkinState galerkinState,
        int pool,
        ArrayList<ElementSamples> samplePool,
        double[] pixelArea,
        ClusterTransportPlan plan)
    {
        if ( interaction.sourceElement == interaction.receiverElement ) {
            return;
        }
        if ( interaction.sourceElement.isCluster() ) {
            plan.setSourceRadiance(ClusterTraversalStrategy.sourceClusterRadiance(interaction, scratch, galerkinState));
        }
        if ( interaction.receiverElement.isCluster()
          && ClusterTraversalStrategy.receiverSamples(interaction, scratch, galerkinState, pixelArea) ) {
            final ArrayList<ElementSamples> visible = scratch.getVisibleElements();
            plan.setReceiverSamples(pixelArea[0], pool, samplePool.size(), visible.size());
            for ( int i = 0; i < visible.size(); i++ ) {
                samplePool.add(visible.get(i));
            }
        }
    }

    /**
    Light transport over the interactions with Z-visibility clustering, as if
    done one after the other in their order. In batches, the workers first render
    what each interaction needs into their own scratch visibility workspaces
    (planClusterTransport); then the transport sums are done serially, in order,
    from those plans.
    Java adaptation: the plans of one batch are reused (reset) by the next,
    where the C++ port allocates them per batch
    */
    private static void applyClusterLightTransports(
        ArrayList<Interaction> transports,
        GalerkinState galerkinState,
        int numberOfThreads)
    {
        final int n = transports.size();
        final int batchSize = 16384;
        final ScratchVisibilityWorkspace[] workspaces = new ScratchVisibilityWorkspace[numberOfThreads];
        // The samples the plans of each worker count; cleared, keeping their
        // storage, from one batch to the next
        final ArrayList<ArrayList<ElementSamples>> samplePools = new ArrayList<>(numberOfThreads);
        for ( int w = 0; w < numberOfThreads; w++ ) {
            workspaces[w] = new ScratchVisibilityWorkspace(galerkinState.scratchFrameBufferSize);
            samplePools.add(new ArrayList<>());
        }
        final ClusterTransportPlan[] plans = new ClusterTransportPlan[Math.min(batchSize, n)];
        for ( int i = 0; i < plans.length; i++ ) {
            plans[i] = new ClusterTransportPlan();
        }
        for ( int batchFirst = 0; batchFirst < n; batchFirst += batchSize ) {
            final int first = batchFirst;
            final int end = Math.min(first + batchSize, n);
            for ( int i = first; i < end; i++ ) {
                plans[i - first].reset();
            }
            final AtomicLong next = new AtomicLong(first);
            runWorkers(numberOfThreads, w -> {
                final ArrayList<ElementSamples> samplePool = samplePools.get(w);
                final double[] pixelArea = new double[1];
                samplePool.clear();
                for ( long i = next.getAndAdd(1); i < end; i = next.getAndAdd(1) ) {
                    planClusterTransport(
                        transports.get((int)i), workspaces[w], galerkinState, w, samplePool, pixelArea, plans[(int)i - first]);
                }
            });
            for ( int i = first; i < end; i++ ) {
                receiverLightTransport(
                    transports.get(i), galerkinState.scratchVisibilityWorkspace, plans[i - first], samplePools, galerkinState);
                sourceLightTransport(transports.get(i), galerkinState);
            }
        }
    }

    /**
    Does what the refinement task recorded, in its order: ids for new children,
    warnings, counting and storing each new link with its receiver, and the events
    of the tasks it forked, where it forked them. The interactions to transport
    light over (new links, and the interaction itself when it was not refined)
    are appended to transports, in the serial order.
    @return true when the task was a top-level interaction that was refined, so
    that it is to be destroyed
    */
    private static boolean commitRefinementTask(
        RefinementScheduler scheduler,
        RefinementTask task,
        RefinementRecord[] records,
        ArrayList<Interaction> transports,
        int[] nextElementId)
    {
        ArrayList<RefinementEvent> events = records[task.getWorker()].getEvents();

        for ( long i = task.getFirstEvent(); i < task.getEndEvent(); i++ ) {
            RefinementEvent event = events.get((int)i);
            switch ( event.getKind() ) {
                case SUBDIVISION: {
                    Element[] children = event.getReceiver().regularSubElements;
                    for ( int j = 0; children != null && j < 4; j++ ) {
                        if ( children[j].id == 0 ) {
                            children[j].id = nextElementId[0]++;
                        }
                    }
                    break;
                }
                case NODES_TOO_CLOSE:
                    FormFactorStrategy.reportNodesTooClose(event.getReceiver(), event.getSource());
                    break;
                case LINK: {
                    Interaction link = event.getLink();
                    Interaction.countCreated(link);
                    transports.add(link);
                    if ( link.receiverElement.interactions == null ) {
                        link.receiverElement.interactions = new ArrayList<>();
                    }
                    link.receiverElement.interactions.add(link);
                    break;
                }
                case FORK:
                    commitRefinementTask(scheduler, scheduler.getForked(event.getTask()), records, transports, nextElementId);
                    break;
                default:
                    break;
            }
        }

        if ( task.getInteraction() != null && !task.isRefined() ) {
            transports.add(task.getInteraction());
        }
        return task.isRefined();
    }

    /**
    Same result as refineInteractions with Jacobi iterations, bit for bit, but
    the refinement and form factors of the interactions are computed by
    GalerkinState.numberOfThreads workers, and what they change (light
    transport, new interactions, element ids, warnings) is then committed in
    the serial order. Configurations whose refinement reads or writes shared
    scratch (non isotropic clustering, shaft culling, multi-resolution
    visibility) are refined serially
    */
    public static void refineInteractionsInParallel(
        Scene scene,
        GalerkinElement parentElement,
        GalerkinState galerkinState)
    {
        if ( !parallelRefinementSupported(galerkinState) ) {
            if ( galerkinState.iterationNumber <= 1 ) {
                Logger.reportMessage(null, Logger.WARNING, "refineInteractionsInParallel", "Parallel Jacobi needs isotropic or Z-visibility clustering, no shaft culling " + "and no multi-resolution visibility: refining serially");
            }
            refineInteractions(scene, parentElement, galerkinState);
            return;
        }
        prepareRefinementIterationState(galerkinState);

        // The interactions of the element lists are the items, in the serial order
        final int numberOfThreads = numberOfRefinementThreads(galerkinState);
        ArrayList<RefinementGroup> groups = new ArrayList<>();
        ArrayList<Interaction> items = new ArrayList<>();
        collectRefinementItems(parentElement, groups, items, new HashSet<>());
        ArrayList<RefinementTask> itemTasks = new ArrayList<>(items.size());
        for ( int i = 0; i < items.size(); i++ ) {
            itemTasks.add(new RefinementTask(items.get(i)));
        }
        final long chunk = Math.max(1L, Math.min(64L, items.size() / (8L * numberOfThreads)));
        final RefinementScheduler scheduler = new RefinementScheduler(itemTasks, chunk, 4L * numberOfThreads);

        // Phase 1: workers refine the items, and the sub-interactions subdivisions fork
        final int firstNewElementId = GalerkinElement.getNumberOfElements() + 1;
        final ReentrantLock subdivisionLock = new ReentrantLock();
        final RefinementRecord[] records = new RefinementRecord[numberOfThreads];
        final ShadowStatistics[] shadowStatistics = new ShadowStatistics[numberOfThreads];
        for ( int w = 0; w < numberOfThreads; w++ ) {
            records[w] = new RefinementRecord();
            shadowStatistics[w] = new ShadowStatistics();
        }
        while ( workerCoefficientsPools.size() < numberOfThreads ) {
            MemoryPool<double[]> pool = new MemoryPool<>(() -> new double[K_SIZE]);
            pool.init(16L * 1024L * 1024L);
            workerCoefficientsPools.add(pool);
        }
        final SceneQuery sceneQuery = scene.getSceneQuery();
        runWorkers(numberOfThreads, w -> {
            SceneQueryWorkspace sceneQueryWorkspace = new SceneQueryWorkspace(sceneQuery);
            FormFactorWorkspace formFactorWorkspace = new FormFactorWorkspace(shadowStatistics[w]);
            formFactorWorkspace.setDeferredNodesTooClose(records[w].getNodesTooClose());
            ScratchVisibilityWorkspace scratchVisibilityWorkspace = null;
            if ( galerkinState.clusteringStrategy == GalerkinClusteringStrategy.Z_VISIBILITY ) {
                scratchVisibilityWorkspace = new ScratchVisibilityWorkspace(galerkinState.scratchFrameBufferSize);
            }
            RefinementWorkspace workspace = new RefinementWorkspace(
                sceneQueryWorkspace,
                formFactorWorkspace,
                scratchVisibilityWorkspace,
                workerCoefficientsPools.get(w),
                records[w],
                subdivisionLock,
                scheduler);
            ArrayList<RefinementEvent> events = records[w].getEvents();
            try {
                long[] range = new long[2];
                scheduler.startItems();
                while ( scheduler.takeItems(range) ) {
                    for ( long i = range[0]; i < range[1]; i++ ) {
                        RefinementTask task = scheduler.getItem(i);
                        final long firstEvent = events.size();
                        final boolean refined = refineInteraction(scene, task.getInteraction(), galerkinState, workspace);
                        task.setResult(w, firstEvent, events.size(), refined);
                    }
                }
                scheduler.done();
                for ( RefinementTask task = scheduler.takeForked(); task != null; task = scheduler.takeForked() ) {
                    final long firstEvent = events.size();
                    refineSubInteraction(
                        scene,
                        task.getCandidatesList(),
                        task.getReceiver(),
                        task.getSource(),
                        galerkinState,
                        workspace);
                    task.setResult(w, firstEvent, events.size(), false);
                    scheduler.done();
                }
            }
            catch ( Throwable error ) {
                error.printStackTrace();
                scheduler.fail();
            }
        });
        for ( int w = 0; w < numberOfThreads; w++ ) {
            Statistics.instance().shadow.numberOfShadowRays += shadowStatistics[w].numberOfShadowRays;
            Statistics.instance().shadow.numberOfShadowCacheHits += shadowStatistics[w].numberOfShadowCacheHits;
        }
        if ( scheduler.hasFailed() ) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "refineInteractionsInParallel", "A refinement worker failed");
        }

        // Phase 2: commit in the serial order; light transport is collected, in
        // that order, and done at the end (in parallel, keeping each element's order)
        ArrayList<Interaction> transports = new ArrayList<>();
        int[] nextElementId = new int[] {firstNewElementId};
        for ( int g = 0; g < groups.size(); g++ ) {
            RefinementGroup group = groups.get(g);
            ArrayList<Object> interactions = group.getElement().interactions;
            int writeIndex = 0;
            for ( int readIndex = 0; readIndex < interactions.size(); readIndex++ ) {
                Interaction interaction = (Interaction)interactions.get(readIndex);
                // A link appended by this element's refinement (readIndex beyond the
                // items) is kept: its light transport was collected when committed
                boolean refined = false;
                if ( readIndex < group.getNumberOfItems() ) {
                    refined = commitRefinementTask(
                        scheduler, scheduler.getItem(group.getFirstItem() + readIndex), records, transports, nextElementId);
                }
                if ( refined ) {
                    Interaction.interactionDestroy(interaction);
                }
                else {
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
        applyLightTransports(transports, galerkinState, numberOfThreads);

        if ( nextElementId[0] != GalerkinElement.getNumberOfElements() + 1 ) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "refineInteractionsInParallel", "Element ids out of the serial order");
        }
    }
}
