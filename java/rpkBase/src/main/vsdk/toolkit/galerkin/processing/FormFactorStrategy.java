/**
All kind of form factor computations
*/

package vsdk.toolkit.galerkin.processing;

import java.util.ArrayList;
import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Matrix2x2;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.statistics.ShadowStatistics;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.galerkin.GalerkinBasis;
import vsdk.toolkit.galerkin.GalerkinClusteringStrategy;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinRole;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.galerkin.Interaction;
import vsdk.toolkit.galerkin.ShadowCache;
import vsdk.toolkit.environment.geometry.elements.RayHitFlag;
import vsdk.toolkit.numericalAnalysis.CubatureRule;
import vsdk.toolkit.scene.SceneQuery;
import vsdk.toolkit.scene.SceneQueryWorkspace;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;
import vsdk.toolkit.skin.RpkGeometry;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RayExclusionSet;
import vsdk.toolkit.environment.geometry.elements.RpkRayHit;

public class FormFactorStrategy {
    private static boolean cachedShadowTest(
        ShadowCache shadowCache,
        Ray ray,
        double shortenedDistance,
        RayExclusionSet exclusions,
        FormFactorWorkspace workspace) {
        double[] distance = workspace.getKernelDistance();
        distance[0] = shortenedDistance;
        return shadowCache.cacheHit(ray, distance, workspace.getKernelHitStore(), exclusions, workspace.getPatchWorkspace()) != null;
    }

    private static RpkRayHit shadowTestDiscretization(
        Ray ray,
        ArrayList<RpkGeometry> geometrySceneList,
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneQueryWorkspace,
        ShadowCache shadowCache,
        double minimumDistance,
        RpkRayHit hitStore,
        boolean isSceneGeometry,
        boolean isClusteredGeometry,
        RayExclusionSet exclusions,
        FormFactorWorkspace workspace)
    {
        ShadowStatistics shadowStatistics = workspace.getShadowStatistics();
        shadowStatistics.numberOfShadowRays++;
        double[] dist = workspace.getShadowTestDistance();
        dist[0] = minimumDistance;
        RpkRayHit hit = shadowCache.cacheHit(ray, dist, hitStore, exclusions, workspace.getPatchWorkspace());
        if ( hit != null ) {
            shadowStatistics.numberOfShadowCacheHits++;
            return hit;
        }

        if ( !isClusteredGeometry && !isSceneGeometry ) {
            hit = RpkGeometry.listDiscretizationIntersect(
                geometrySceneList,
                ray,
                Numeric.EPSILON * dist[0],
                dist,
                RayHitFlag.FRONT | RayHitFlag.ANY,
                hitStore,
                exclusions);
        }
        else if ( sceneQuery != null ) {
            hit = sceneQuery.intersect(
                ray,
                Numeric.EPSILON * dist[0],
                dist,
                RayHitFlag.FRONT | RayHitFlag.ANY,
                hitStore,
                exclusions,
                sceneQueryWorkspace);
        }

        if ( hit != null && hit.getPatch() != null ) {
            shadowCache.addToShadowCache(hit.getPatch());
        }
        return hit;
    }

    /**
Warns that two cubature nodes of an element pair are too close to interact
*/
    public static void reportNodesTooClose(GalerkinElement receiverElement, GalerkinElement sourceElement) {
        Logger.reportMessage(null, Logger.WARNING, "evaluatePointsPairKernel", String.format("Nodes too close too each other (receiver id %d, source id %d)", receiverElement.id, sourceElement.id));
    }

    /**
This routine determines the cubature rule to be used on the given element
and computes the positions on the elements patch that correspond to the nodes
of the cubature rule on the element. The role (RECEIVER or SOURCE) is only
relevant for surface elements
*/
    @SuppressWarnings("unused")
    private static void determineNodes(
        GalerkinElement element,
        GalerkinRole role,
        GalerkinState galerkinState,
        CubatureRule[] cr,
        Vector3Dd[] x)
    {
        if ( element == null || cr == null || cr.length == 0 || x == null ) {
            return;
        }

        if ( element.isCluster() ) {
            cr[0] = galerkinState.clusterRule;
            RpkAxisAlignedBoundingBox bb = new RpkAxisAlignedBoundingBox();
            element.bounds(bb);
            double dx = bb.dx();
            double dy = bb.dy();
            double dz = bb.dz();
            double minX = bb.minX();
            double minY = bb.minY();
            double minZ = bb.minZ();
            for ( int k = 0; cr[0] != null && k < cr[0].numberOfNodes && k < x.length; k++ ) {
                if ( x[k] == null ) {
                    x[k] = new Vector3Dd();
                }
                x[k] = new Vector3Dd(minX + (double)(cr[0].u[k] * dx), minY + (double)(cr[0].v[k] * dy), minZ + (double)(cr[0].t[k] * dz));
            }
            return;
        }

        switch ( element.patch.getNumberOfVertices() ) {
            case 3:
                cr[0] = (role == GalerkinRole.RECEIVER)
                    ? galerkinState.receiverTriangleCubatureRule
                    : galerkinState.sourceTriangleCubatureRule;
                break;
            case 4:
                cr[0] = (role == GalerkinRole.RECEIVER)
                    ? galerkinState.receiverQuadCubatureRule
                    : galerkinState.sourceQuadCubatureRule;
                break;
            default:
                cr[0] = null;
                return;
        }

        Matrix2x2 topTransform = element.transformToParent != null ? element.topTransform() : null;
        boolean hasTopTransform = topTransform != null;

        for ( int k = 0; cr[0] != null && k < cr[0].numberOfNodes && k < x.length; k++ ) {
            Vector2Dd node = new Vector2Dd((double)cr[0].u[k], (double)cr[0].v[k]);
            if ( hasTopTransform ) {
                node = topTransform.transformPoint(node);
            }
            x[k] = element.patch.uniformPoint(node.x(), node.y());
        }
    }

    private static double evaluatePointsPairKernel(
        ShadowCache shadowCache,
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneQueryWorkspace,
        Vector3Dd x,
        Vector3Dd y,
        GalerkinElement receiverElement,
        GalerkinElement sourceElement,
        ArrayList<RpkGeometry> shadowGeometryList,
        boolean isSceneGeometry,
        boolean isClusteredGeometry,
        GalerkinState galerkinState,
        RayExclusionSet exclusions,
        FormFactorWorkspace workspace)
    {
        // Trace the ray from source to receiver (y to x) to handle one-sided surfaces correctly
        Ray ray = Ray.fromAffine(y, x.subtract(y));
        double distance = ray.getDirection().length();

        // Don't allow too nearby nodes to interact
        if ( distance < VSDK.EPSILON ) {
            if ( !workspace.deferNodesTooClose(receiverElement, sourceElement) ) {
                reportNodesTooClose(receiverElement, sourceElement);
            }
            return 0.0;
        }
        ray = ray.withDirection(ray.getDirection().multiplyByInverse((double)distance, Numeric.EPSILON));

        double cosThetaY;
        if ( sourceElement.isCluster() ) {
            cosThetaY = 0.25;
        }
        else {
            cosThetaY = ray.getDirection().dotProduct(sourceElement.patch.getNormal());
            if ( cosThetaY <= 0.0 ) {
                return 0.0;
            }
        }

        double cosThetaX;
        if ( receiverElement.isCluster() ) {
            cosThetaX = 0.25;
        }
        else {
            cosThetaX = -ray.getDirection().dotProduct(receiverElement.patch.getNormal());
            if ( cosThetaX <= 0.0 ) {
                return 0.0;
            }
        }

        double formFactorKernelTerm = cosThetaX * cosThetaY / (Math.PI * distance * distance);
        double shortenedDistance = (double)(distance * (1.0 - VSDK.EPSILON));

        double visibilityFactor;
        if ( shadowGeometryList == null || shadowGeometryList.isEmpty() ) {
            visibilityFactor = 1.0;
        }
        else if ( galerkinState.multiResolutionVisibility == 0 ) {
            RpkRayHit hitStore = workspace.getKernelHitStore();
            RpkRayHit h = shadowTestDiscretization(
                ray,
                shadowGeometryList,
                sceneQuery,
                sceneQueryWorkspace,
                shadowCache,
                shortenedDistance,
                hitStore,
                isSceneGeometry,
                isClusteredGeometry,
                exclusions,
                workspace);
            visibilityFactor = (h == null) ? 1.0 : 0.0;
        }
        else if ( cachedShadowTest(shadowCache, ray, shortenedDistance, exclusions, workspace) ) {
            visibilityFactor = 0.0;
        }
        else {
            double minimumFeatureSize = 2.0
                * (double)Math.sqrt(Statistics.instance().radiance.totalArea * galerkinState.relMinElemArea / Math.PI);
            visibilityFactor = FormFactorClusteredStrategy.geomListMultiResolutionVisibility(
                shadowGeometryList,
                shadowCache,
                ray,
                shortenedDistance,
                sourceElement.blockerSize,
                minimumFeatureSize,
                exclusions);
        }

        return formFactorKernelTerm * visibilityFactor;
    }

    public static void computeAreaToAreaFormFactorVisibility(
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneQueryWorkspace,
        FormFactorWorkspace formFactorWorkspace,
        ArrayList<RpkGeometry> geometryShadowList,
        boolean isSceneGeometry,
        boolean isClusteredGeometry,
        Interaction link,
        GalerkinState galerkinState)
    {
        GalerkinElement receiverElement = link.receiverElement;
        GalerkinElement sourceElement = link.sourceElement;
        if ( receiverElement == null || sourceElement == null ) {
            link.visibility = 0;
            return;
        }

        int m = link.numberOfBasisFunctionsOnReceiver * link.numberOfBasisFunctionsOnSource;
        if ( m <= 0 ) {
            m = 1;
        }
        if ( link.K == null || link.K.length < m ) {
            link.K = new double[m];
        }
        for ( int i = 0; i < m; i++ ) {
            link.K[i] = 0.0;
        }

        if ( receiverElement.isCluster() || sourceElement.isCluster() ) {
            RpkAxisAlignedBoundingBox sourceBoundingBox = new RpkAxisAlignedBoundingBox();
            RpkAxisAlignedBoundingBox receiverBoundingBox = new RpkAxisAlignedBoundingBox();
            receiverElement.bounds(receiverBoundingBox);
            sourceElement.bounds(sourceBoundingBox);
            if ( !receiverBoundingBox.disjointToOtherBoundingBox(sourceBoundingBox) ) {
                link.deltaK = 1.0;
                link.numberOfReceiverCubaturePositions = 1;
                link.visibility = (byte)128;
                return;
            }
        }
        else if ( receiverElement == sourceElement ) {
            link.deltaK = 0.0;
            link.numberOfReceiverCubaturePositions = 1;
            link.visibility = 0;
            return;
        }

        // If the receiver is another one than before, determine the cubature rule to be
        // used on it and the nodes (positions on the patch); same for the source element
        // The cubature rules, nodes and kernel values of the last pair are kept
        // in the form factor workspace
        Vector3Dd[] cachedX = formFactorWorkspace.getReceiverNodes();
        Vector3Dd[] cachedY = formFactorWorkspace.getSourceNodes();
        double[][] cachedGxy = formFactorWorkspace.getKernel();
        final boolean sameReceiver = receiverElement == formFactorWorkspace.getLastReceiver();
        final boolean sameSource = sourceElement == formFactorWorkspace.getLastSource();
        if ( !sameReceiver ) {
            CubatureRule[] receiverCubatureRuleRef = new CubatureRule[] {formFactorWorkspace.getReceiverCubatureRule()};
            determineNodes(receiverElement, GalerkinRole.RECEIVER, galerkinState, receiverCubatureRuleRef, cachedX);
            formFactorWorkspace.setReceiverCubatureRule(receiverCubatureRuleRef[0]);
        }
        if ( !sameSource ) {
            CubatureRule[] sourceCubatureRuleRef = new CubatureRule[] {formFactorWorkspace.getSourceCubatureRule()};
            determineNodes(sourceElement, GalerkinRole.SOURCE, galerkinState, sourceCubatureRuleRef, cachedY);
            formFactorWorkspace.setSourceCubatureRule(sourceCubatureRuleRef[0]);
        }

        CubatureRule receiverCubatureRule = formFactorWorkspace.getReceiverCubatureRule();
        CubatureRule sourceCubatureRule = formFactorWorkspace.getSourceCubatureRule();
        if ( receiverCubatureRule == null || sourceCubatureRule == null ) {
            link.deltaK = 0.0;
            link.numberOfReceiverCubaturePositions = 1;
            link.visibility = 0;
            return;
        }

        // Evaluate the radiosity kernel between each pair of nodes on the source and the
        // receiver element only if at least the receiver or the source changed since last time
        if ( !sameReceiver || !sameSource ) {
            ShadowCache shadowCache = new ShadowCache();
            // Exclude the patches in order to avoid immediate self-intersections
            RayExclusionSet exclusions = new RayExclusionSet(
                receiverElement.isCluster() ? null : receiverElement.patch,
                (receiverElement.isCluster() || receiverElement.patch == null) ? null : receiverElement.patch.getTwin(),
                sourceElement.isCluster() ? null : sourceElement.patch,
                (sourceElement.isCluster() || sourceElement.patch == null) ? null : sourceElement.patch.getTwin(),
                receiverElement.isCluster() ? receiverElement.geometry : null,
                sourceElement.isCluster() ? sourceElement.geometry : null);


            double cachedMaximumKernelValue = 0.0;
            int cachedVisibilityCount = 0;
            for ( int r = 0; r < receiverCubatureRule.numberOfNodes; r++ ) {
                for ( int s = 0; s < sourceCubatureRule.numberOfNodes; s++ ) {
                    cachedGxy[r][s] = evaluatePointsPairKernel(
                        shadowCache,
                        sceneQuery,
                        sceneQueryWorkspace,
                        cachedX[r],
                        cachedY[s],
                        receiverElement,
                        sourceElement,
                        geometryShadowList,
                        isSceneGeometry,
                        isClusteredGeometry,
                        galerkinState,
                        exclusions,
                        formFactorWorkspace);
                    if ( cachedGxy[r][s] > cachedMaximumKernelValue ) {
                        cachedMaximumKernelValue = cachedGxy[r][s];
                    }
                    if ( Math.abs(cachedGxy[r][s]) > VSDK.EPSILON ) {
                        cachedVisibilityCount++;
                    }
                }
            }

            formFactorWorkspace.setMaximumKernelValue(cachedMaximumKernelValue);
            formFactorWorkspace.setVisibilityCount(cachedVisibilityCount);
            boundPointToAreaFormFactors(receiverCubatureRule, sourceCubatureRule, sourceElement.area, cachedGxy);
        }

        double maximumKernelValue = formFactorWorkspace.getMaximumKernelValue();
        double[][] Gxy = cachedGxy;
        int visibilityCount = formFactorWorkspace.getVisibilityCount();

        if ( visibilityCount != 0 ) {
            if ( link.numberOfBasisFunctionsOnReceiver == 1 && link.numberOfBasisFunctionsOnSource == 1 ) {
                FormFactorClusteredStrategy.doConstantAreaToAreaFormFactor(
                    link, receiverCubatureRule, sourceCubatureRule, Gxy);
            }
            else {
                doHigherOrderAreaToAreaFormFactor(
                    link, receiverCubatureRule, sourceCubatureRule, Gxy, galerkinState);
            }
        }
        else {
            link.K[0] = 0.0;
            link.deltaK = 0.0;
            link.numberOfReceiverCubaturePositions = 1;
        }

        link.visibility = (byte)((255.0 * visibilityCount)
            / (receiverCubatureRule.numberOfNodes * sourceCubatureRule.numberOfNodes));
        if ( galerkinState.exactVisibility != 0 && geometryShadowList != null && link.visibility == (byte)255 ) {
            link.visibility = (byte)254;
        }

        if ( galerkinState.clusteringStrategy == GalerkinClusteringStrategy.ISOTROPIC
            && (receiverElement.isCluster() || sourceElement.isCluster()) ) {
            link.deltaK = (double)(maximumKernelValue * sourceElement.area);
            link.numberOfReceiverCubaturePositions = 1;
        }
        formFactorWorkspace.setLastPair(receiverElement, sourceElement);
    }

    /**
    Bounds the point-to-area form factor at every receiver node by 1. The form
    factor from a point x_k to the source, G(x_k) = A_s sum_l w_l G(x_k, y_l), is
    the fraction of the hemisphere over x_k that the source covers, so it never
    exceeds 1: summed over all sources it is at most 1, the bound [BEKA1996]
    section 4.1 states after its equation (11). The cubature estimate is the
    imprecise numerical integration of [BEKA1996] section 7, and it exceeds it when
    nodes of two elements nearly touch (elements meeting at an edge or a corner),
    where 1 / (pi r^2) explodes. With elements already at the minimum area the
    refinement cannot remove such a link, and a link with G > 1 and its reciprocal
    form a feedback loop of gain above 1 that makes the solution diverge (a tiny
    element turns into a bright emitter). The kernel values of a receiver node
    whose estimate exceeds 1 are scaled by the same factor, so that the node's
    estimate is 1 and its shape over the source (the higher order coefficients
    and the error estimate) is kept. Legitimate estimates are below 1 and stay
    untouched. With nodes on cluster bounding boxes (0.25 cosine terms) the same
    bound holds
    */
    static void boundPointToAreaFormFactors(
        CubatureRule receiverCubatureRule,
        CubatureRule sourceCubatureRule,
        double sourceArea,
        double[][] Gxy)
    {
        for ( int k = 0; receiverCubatureRule != null && sourceCubatureRule != null && k < receiverCubatureRule.numberOfNodes; k++ ) {
            double g = 0.0;
            for ( int l = 0; l < sourceCubatureRule.numberOfNodes; l++ ) {
                g += sourceCubatureRule.w[l] * Gxy[k][l];
            }
            g *= sourceArea;
            if ( g > 1.0 ) {
                final double scale = 1.0 / g;
                for ( int l = 0; l < sourceCubatureRule.numberOfNodes; l++ ) {
                    Gxy[k][l] *= scale;
                }
            }
        }
    }

    @SuppressWarnings("unused")
    private static void doHigherOrderAreaToAreaFormFactor(
        Interaction twoPatchesInteraction,
        CubatureRule receiverCubatureRule,
        CubatureRule sourceCubatureRule,
        double[][] Gxy,
        GalerkinState galerkinState)
    {
        GalerkinElement receiverElement = twoPatchesInteraction.receiverElement;
        GalerkinElement sourceElement = twoPatchesInteraction.sourceElement;

        GalerkinBasis receiverBasis = receiverElement.isCluster()
            ? null
            : GalerkinBasis.basisForVertexCount(receiverElement.patch.getNumberOfVertices());
        GalerkinBasis sourceBasis = sourceElement.isCluster()
            ? null
            : GalerkinBasis.basisForVertexCount(sourceElement.patch.getNumberOfVertices());

        ColorRgb[] sourceRadiance = (galerkinState.galerkinIterationMethod == vsdk.toolkit.galerkin.GalerkinIterationMethod.SOUTH_WELL)
            ? sourceElement.unShotRadiance
            : sourceElement.radiance;

        ColorRgb[] deltaRadiance = new ColorRgb[CubatureRule.MAXIMUM_NODES];
        for ( int i = 0; i < deltaRadiance.length; i++ ) {
            deltaRadiance[i] = new ColorRgb();
        }
        double[] gMin = new double[] {Numeric.HUGE_DOUBLE_VALUE};
        double[] gMax = new double[] {-Numeric.HUGE_DOUBLE_VALUE};

        computeInteractionFormFactor(
            receiverCubatureRule,
            sourceCubatureRule,
            Gxy,
            sourceElement,
            receiverElement,
            sourceBasis,
            receiverBasis,
            sourceRadiance,
            gMin,
            gMax,
            deltaRadiance,
            twoPatchesInteraction);

        computeInteractionError(
            receiverCubatureRule,
            receiverElement,
            gMin[0],
            gMax[0],
            sourceRadiance,
            deltaRadiance,
            twoPatchesInteraction);
    }

    @SuppressWarnings("unused")
    private static void computeInteractionError(
        CubatureRule receiverCubatureRule,
        GalerkinElement receiverElement,
        double gMin,
        double gMax,
        ColorRgb[] sourceRadiance,
        ColorRgb[] deltaRadiance,
        Interaction link)
    {
        if ( sourceRadiance[0].isBlack() ) {
            double gAverage = link.K[0] / receiverElement.area;
            link.deltaK = (double)(gMax - gAverage);
            if ( gAverage - gMin > link.deltaK ) {
                link.deltaK = (double)(gAverage - gMin);
            }
        }
        else {
            link.deltaK = 0.0;
            for ( int k = 0; k < receiverCubatureRule.numberOfNodes; k++ ) {
                deltaRadiance[k] = deltaRadiance[k].divide(sourceRadiance[0]);
                double delta = Math.abs(deltaRadiance[k].maximumComponent());
                if ( delta > link.deltaK ) {
                    link.deltaK = (double)delta;
                }
            }
        }
        link.numberOfReceiverCubaturePositions = 1;
    }

    @SuppressWarnings("unused")
    private static void computeInteractionFormFactor(
        CubatureRule receiverCubatureRule,
        CubatureRule sourceCubatureRule,
        double[][] Gxy,
        GalerkinElement sourceElement,
        GalerkinElement receiverElement,
        vsdk.toolkit.galerkin.GalerkinBasis sourceBasis,
        vsdk.toolkit.galerkin.GalerkinBasis receiverBasis,
        ColorRgb[] sourceRadiance,
        double[] gMin,
        double[] gMax,
        ColorRgb[] deltaRadiance,
        Interaction twoPatchesInteraction)
    {
        double[][] receiverPhi = new double[GalerkinBasis.MAX_BASIS_SIZE][CubatureRule.MAXIMUM_NODES];
        for ( int k = 0; k < receiverCubatureRule.numberOfNodes; k++ ) {
            if ( receiverElement.isCluster() ) {
                receiverPhi[0][k] = 1.0;
            }
            else {
                for ( int alpha = 0; alpha < twoPatchesInteraction.numberOfBasisFunctionsOnReceiver && receiverBasis != null; alpha++ ) {
                    receiverPhi[alpha][k] = receiverBasis.function[alpha].evaluate(receiverCubatureRule.u[k], receiverCubatureRule.v[k]);
                }
            }
        }

        double[] sourcePhi = new double[CubatureRule.MAXIMUM_NODES];
        for ( int k = 0; k < receiverCubatureRule.numberOfNodes; k++ ) {
            deltaRadiance[k] = new ColorRgb(0.0, 0.0, 0.0);
        }

        for ( int beta = 0; beta < twoPatchesInteraction.numberOfBasisFunctionsOnSource; beta++ ) {
            if ( sourceElement.isCluster() ) {
                for ( int l = 0; l < sourceCubatureRule.numberOfNodes; l++ ) {
                    sourcePhi[l] = 1.0;
                }
            }
            else {
                for ( int l = 0; l < sourceCubatureRule.numberOfNodes && sourceBasis != null; l++ ) {
                    sourcePhi[l] = sourceBasis.function[beta].evaluate(sourceCubatureRule.u[l], sourceCubatureRule.v[l]);
                }
            }

            double[] gBeta = new double[CubatureRule.MAXIMUM_NODES];
            double[] deltaBeta = new double[CubatureRule.MAXIMUM_NODES];
            for ( int k = 0; k < receiverCubatureRule.numberOfNodes; k++ ) {
                gBeta[k] = 0.0;
                for ( int l = 0; l < sourceCubatureRule.numberOfNodes; l++ ) {
                    gBeta[k] += sourceCubatureRule.w[l] * Gxy[k][l] * sourcePhi[l];
                }
                gBeta[k] *= sourceElement.area;
                deltaBeta[k] = -gBeta[k];
            }

            for ( int alpha = 0; alpha < twoPatchesInteraction.numberOfBasisFunctionsOnReceiver; alpha++ ) {
                double gAlphaBeta = 0.0;
                for ( int k = 0; k < receiverCubatureRule.numberOfNodes; k++ ) {
                    gAlphaBeta += receiverCubatureRule.w[k] * receiverPhi[alpha][k] * gBeta[k];
                }
                twoPatchesInteraction.K[alpha * twoPatchesInteraction.numberOfBasisFunctionsOnSource + beta] =
                    (double)(receiverElement.area * gAlphaBeta);
                for ( int k = 0; k < receiverCubatureRule.numberOfNodes; k++ ) {
                    deltaBeta[k] += gAlphaBeta * receiverPhi[alpha][k];
                }
            }

            for ( int k = 0; k < receiverCubatureRule.numberOfNodes; k++ ) {
                deltaRadiance[k] = deltaRadiance[k].addScaled((double)deltaBeta[k], sourceRadiance[beta]);
            }

            if ( beta == 0 ) {
                for ( int k = 0; k < receiverCubatureRule.numberOfNodes; k++ ) {
                    if ( gBeta[k] < gMin[0] ) {
                        gMin[0] = gBeta[k];
                    }
                    if ( gBeta[k] > gMax[0] ) {
                        gMax[0] = gBeta[k];
                    }
                }
            }
        }
    }
}
