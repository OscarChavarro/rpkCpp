/**
All kind of form factor computations
*/

package vsdk.toolkit.galerkin.processing;

import java.util.ArrayList;
import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.linealAlgebra.Matrix2x2;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector2Df;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
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
import vsdk.toolkit.scene.VisibilityQueryRecorder;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RayExclusionSet;
import vsdk.toolkit.environment.geometry.elements.RayHit;

public class FormFactorStrategy {
    private static boolean cachedShadowTest(
        ShadowCache shadowCache,
        Ray ray,
        float shortenedDistance,
        RayExclusionSet exclusions,
        FormFactorWorkspace workspace) {
        float[] distance = workspace.getKernelDistance();
        distance[0] = shortenedDistance;
        return shadowCache.cacheHit(ray, distance, workspace.getKernelHitStore(), exclusions, workspace.getPatchWorkspace()) != null;
    }

    private static RayHit shadowTestDiscretization(
        Ray ray,
        ArrayList<Geometry> geometrySceneList,
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneQueryWorkspace,
        ShadowCache shadowCache,
        float minimumDistance,
        RayHit hitStore,
        boolean isSceneGeometry,
        boolean isClusteredGeometry,
        RayExclusionSet exclusions,
        FormFactorWorkspace workspace)
    {
        ShadowStatistics shadowStatistics = workspace.getShadowStatistics();
        shadowStatistics.numberOfShadowRays++;
        float[] dist = workspace.getShadowTestDistance();
        dist[0] = minimumDistance;
        RayHit hit = shadowCache.cacheHit(ray, dist, hitStore, exclusions, workspace.getPatchWorkspace());
        if ( hit != null ) {
            shadowStatistics.numberOfShadowCacheHits++;
            if ( sceneQueryWorkspace != null && sceneQueryWorkspace.getRecorder() != null ) {
                sceneQueryWorkspace.getRecorder().countCacheResolved();
            }
            return hit;
        }

        final float queryMaximumDistance = dist[0];
        if ( !isClusteredGeometry && !isSceneGeometry ) {
            hit = Geometry.listDiscretizationIntersect(
                geometrySceneList,
                ray,
                Numeric.EPSILON_FLOAT * dist[0],
                dist,
                RayHitFlag.FRONT | RayHitFlag.ANY,
                hitStore,
                exclusions);
        }
        else if ( sceneQuery != null ) {
            hit = sceneQuery.intersect(
                ray,
                Numeric.EPSILON_FLOAT * dist[0],
                dist,
                RayHitFlag.FRONT | RayHitFlag.ANY,
                hitStore,
                exclusions,
                sceneQueryWorkspace);
        }
        if ( sceneQueryWorkspace != null && sceneQueryWorkspace.getRecorder() != null ) {
            sceneQueryWorkspace.getRecorder().recordQuery(
                (!isClusteredGeometry && !isSceneGeometry) ?
                    VisibilityQueryRecorder.GALERKIN_RESTRICTED_LIST : VisibilityQueryRecorder.GALERKIN_FULL_SCENE,
                ray,
                Numeric.EPSILON_FLOAT * queryMaximumDistance,
                queryMaximumDistance,
                RayHitFlag.FRONT | RayHitFlag.ANY,
                exclusions,
                hit,
                dist[0]);
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
        Vector3Df[] x)
    {
        if ( element == null || cr == null || cr.length == 0 || x == null ) {
            return;
        }

        if ( element.isCluster() ) {
            cr[0] = galerkinState.clusterRule;
            BoundingBox bb = new BoundingBox();
            element.bounds(bb);
            double dx = bb.dx();
            double dy = bb.dy();
            double dz = bb.dz();
            float minX = bb.minX();
            float minY = bb.minY();
            float minZ = bb.minZ();
            for ( int k = 0; cr[0] != null && k < cr[0].numberOfNodes && k < x.length; k++ ) {
                if ( x[k] == null ) {
                    x[k] = new Vector3Df();
                }
                x[k] = new Vector3Df(minX + (float)(cr[0].u[k] * dx), minY + (float)(cr[0].v[k] * dy), minZ + (float)(cr[0].t[k] * dz));
            }
            return;
        }

        switch ( element.patch.numberOfVertices ) {
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
            Vector2Df node = new Vector2Df((float)cr[0].u[k], (float)cr[0].v[k]);
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
        Vector3Df x,
        Vector3Df y,
        GalerkinElement receiverElement,
        GalerkinElement sourceElement,
        ArrayList<Geometry> shadowGeometryList,
        boolean isSceneGeometry,
        boolean isClusteredGeometry,
        GalerkinState galerkinState,
        RayExclusionSet exclusions,
        FormFactorWorkspace workspace)
    {
        Ray ray = workspace.getKernelRay();
        ray.position = y;
        ray.direction = x.subtract(y);
        double distance = ray.direction.length();

        // Don't allow too nearby nodes to interact
        if ( distance < VSDK.EPSILON ) {
            if ( !workspace.deferNodesTooClose(receiverElement, sourceElement) ) {
                reportNodesTooClose(receiverElement, sourceElement);
            }
            return 0.0;
        }
        ray.direction = ray.direction.multiplyByInverse((float)distance, Numeric.EPSILON_FLOAT);

        double cosThetaY;
        if ( sourceElement.isCluster() ) {
            cosThetaY = 0.25;
        }
        else {
            cosThetaY = ray.direction.dotProduct(sourceElement.patch.getNormal());
            if ( cosThetaY <= 0.0 ) {
                return 0.0;
            }
        }

        double cosThetaX;
        if ( receiverElement.isCluster() ) {
            cosThetaX = 0.25;
        }
        else {
            cosThetaX = -ray.direction.dotProduct(receiverElement.patch.getNormal());
            if ( cosThetaX <= 0.0 ) {
                return 0.0;
            }
        }

        double formFactorKernelTerm = cosThetaX * cosThetaY / (Math.PI * distance * distance);
        float shortenedDistance = (float)(distance * (1.0 - VSDK.EPSILON));

        double visibilityFactor;
        if ( shadowGeometryList == null || shadowGeometryList.isEmpty() ) {
            visibilityFactor = 1.0;
        }
        else if ( galerkinState.multiResolutionVisibility == 0 ) {
            RayHit hitStore = workspace.getKernelHitStore();
            RayHit h = shadowTestDiscretization(
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
            if ( sceneQueryWorkspace != null && sceneQueryWorkspace.getRecorder() != null ) {
                sceneQueryWorkspace.getRecorder().countCacheResolved();
            }
            visibilityFactor = 0.0;
        }
        else {
            float minimumFeatureSize = 2.0f
                * (float)Math.sqrt(Statistics.instance().radiance.totalArea * galerkinState.relMinElemArea / Math.PI);
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
        ArrayList<Geometry> geometryShadowList,
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
            link.K = new float[m];
        }
        for ( int i = 0; i < m; i++ ) {
            link.K[i] = 0.0f;
        }

        if ( receiverElement.isCluster() || sourceElement.isCluster() ) {
            BoundingBox sourceBoundingBox = new BoundingBox();
            BoundingBox receiverBoundingBox = new BoundingBox();
            receiverElement.bounds(receiverBoundingBox);
            sourceElement.bounds(sourceBoundingBox);
            if ( !receiverBoundingBox.disjointToOtherBoundingBox(sourceBoundingBox) ) {
                link.deltaK = 1.0f;
                link.numberOfReceiverCubaturePositions = 1;
                link.visibility = (byte)128;
                return;
            }
        }
        else if ( receiverElement == sourceElement ) {
            link.deltaK = 0.0f;
            link.numberOfReceiverCubaturePositions = 1;
            link.visibility = 0;
            return;
        }

        // If the receiver is another one than before, determine the cubature rule to be
        // used on it and the nodes (positions on the patch); same for the source element
        // The cubature rules, nodes and kernel values of the last pair are kept
        // in the form factor workspace
        Vector3Df[] cachedX = formFactorWorkspace.getReceiverNodes();
        Vector3Df[] cachedY = formFactorWorkspace.getSourceNodes();
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
            link.deltaK = 0.0f;
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
                (receiverElement.isCluster() || receiverElement.patch == null) ? null : receiverElement.patch.twin,
                sourceElement.isCluster() ? null : sourceElement.patch,
                (sourceElement.isCluster() || sourceElement.patch == null) ? null : sourceElement.patch.twin,
                receiverElement.isCluster() ? receiverElement.geometry : null,
                sourceElement.isCluster() ? sourceElement.geometry : null);

            if ( sceneQueryWorkspace != null && sceneQueryWorkspace.getRecorder() != null
              && geometryShadowList != null && !geometryShadowList.isEmpty()
              && !isClusteredGeometry && !isSceneGeometry ) {
                sceneQueryWorkspace.getRecorder().recordGeometryList(geometryShadowList);
            }

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
            link.K[0] = 0.0f;
            link.deltaK = 0.0f;
            link.numberOfReceiverCubaturePositions = 1;
        }

        link.visibility = (byte)((255.0 * visibilityCount)
            / (receiverCubatureRule.numberOfNodes * sourceCubatureRule.numberOfNodes));
        if ( galerkinState.exactVisibility != 0 && geometryShadowList != null && link.visibility == (byte)255 ) {
            link.visibility = (byte)254;
        }

        if ( galerkinState.clusteringStrategy == GalerkinClusteringStrategy.ISOTROPIC
            && (receiverElement.isCluster() || sourceElement.isCluster()) ) {
            link.deltaK = (float)(maximumKernelValue * sourceElement.area);
            link.numberOfReceiverCubaturePositions = 1;
        }
        formFactorWorkspace.setLastPair(receiverElement, sourceElement);
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
            : GalerkinBasis.basisForVertexCount(receiverElement.patch.numberOfVertices);
        GalerkinBasis sourceBasis = sourceElement.isCluster()
            ? null
            : GalerkinBasis.basisForVertexCount(sourceElement.patch.numberOfVertices);

        ColorRgbMutable[] sourceRadiance = (galerkinState.galerkinIterationMethod == vsdk.toolkit.galerkin.GalerkinIterationMethod.SOUTH_WELL)
            ? sourceElement.unShotRadiance
            : sourceElement.radiance;

        ColorRgbMutable[] deltaRadiance = new ColorRgbMutable[CubatureRule.MAXIMUM_NODES];
        for ( int i = 0; i < deltaRadiance.length; i++ ) {
            deltaRadiance[i] = new ColorRgbMutable();
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
        ColorRgbMutable[] sourceRadiance,
        ColorRgbMutable[] deltaRadiance,
        Interaction link)
    {
        if ( sourceRadiance[0].isBlack() ) {
            double gAverage = link.K[0] / receiverElement.area;
            link.deltaK = (float)(gMax - gAverage);
            if ( gAverage - gMin > link.deltaK ) {
                link.deltaK = (float)(gAverage - gMin);
            }
        }
        else {
            link.deltaK = 0.0f;
            for ( int k = 0; k < receiverCubatureRule.numberOfNodes; k++ ) {
                deltaRadiance[k].divide(deltaRadiance[k], sourceRadiance[0]);
                double delta = Math.abs(deltaRadiance[k].maximumComponent());
                if ( delta > link.deltaK ) {
                    link.deltaK = (float)delta;
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
        ColorRgbMutable[] sourceRadiance,
        double[] gMin,
        double[] gMax,
        ColorRgbMutable[] deltaRadiance,
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
            deltaRadiance[k].clear();
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
                    (float)(receiverElement.area * gAlphaBeta);
                for ( int k = 0; k < receiverCubatureRule.numberOfNodes; k++ ) {
                    deltaBeta[k] += gAlphaBeta * receiverPhi[alpha][k];
                }
            }

            for ( int k = 0; k < receiverCubatureRule.numberOfNodes; k++ ) {
                deltaRadiance[k].addScaled(deltaRadiance[k], (float)deltaBeta[k], sourceRadiance[beta]);
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
