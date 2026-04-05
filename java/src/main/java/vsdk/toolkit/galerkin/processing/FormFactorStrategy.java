/**
All kind of form factor computations
*/

package vsdk.toolkit.galerkin.processing;

import java.util.ArrayList;
import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Matrix2x2;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector2D;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.galerkin.GalerkinBasis;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinRole;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.galerkin.Interaction;
import vsdk.toolkit.galerkin.ShadowCache;
import vsdk.toolkit.material.RayHitFlag;
import vsdk.toolkit.numericalAnalysis.CubatureRule;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.RayHit;

public class FormFactorStrategy {
    // Global variables used for form factor computation optimisation
    private static GalerkinElement formFactorLastReceived;
    private static GalerkinElement formFactorLastSource;

    private static RayHit shadowTestDiscretization(
        Ray ray,
        ArrayList<Geometry> geometrySceneList,
        VoxelGrid voxelGrid,
        ShadowCache shadowCache,
        float minimumDistance,
        RayHit hitStore,
        boolean isSceneGeometry,
        boolean isClusteredGeometry)
    {
        Statistics.instance().shadow.numberOfShadowRays++;
        float[] dist = new float[] {minimumDistance};
        RayHit hit = shadowCache.cacheHit(ray, dist, hitStore);
        if ( hit != null ) {
            Statistics.instance().shadow.numberOfShadowCacheHits++;
            return hit;
        }

        if ( !isClusteredGeometry && !isSceneGeometry ) {
            hit = Geometry.listDiscretizationIntersect(
                geometrySceneList,
                ray,
                Numeric.EPSILON_FLOAT * minimumDistance,
                dist,
                RayHitFlag.FRONT | RayHitFlag.ANY,
                hitStore);
        }
        else if ( voxelGrid != null ) {
            hit = voxelGrid.gridIntersect(
                ray,
                Numeric.EPSILON_FLOAT * minimumDistance,
                dist,
                RayHitFlag.FRONT | RayHitFlag.ANY,
                hitStore);
        }

        if ( hit != null && hit.getPatch() != null ) {
            shadowCache.addToShadowCache(hit.getPatch());
        }
        return hit;
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
        Vector3D[] x)
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
                    x[k] = new Vector3D();
                }
                x[k].set(
                    minX + (float)(cr[0].u[k] * dx),
                    minY + (float)(cr[0].v[k] * dy),
                    minZ + (float)(cr[0].t[k] * dz));
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

        Matrix2x2 topTransform = new Matrix2x2();
        boolean hasTopTransform = element.transformToParent != null && element.topTransform(topTransform) != null;

        for ( int k = 0; cr[0] != null && k < cr[0].numberOfNodes && k < x.length; k++ ) {
            if ( x[k] == null ) {
                x[k] = new Vector3D();
            }
            Vector2D node = new Vector2D((float)cr[0].u[k], (float)cr[0].v[k]);
            if ( hasTopTransform ) {
                topTransform.transformPoint2D(node, node);
            }
            element.patch.uniformPoint(node.x, node.y, x[k]);
        }
    }

    private static double evaluatePointsPairKernel(
        ShadowCache shadowCache,
        VoxelGrid sceneWorldVoxelGrid,
        Vector3D x,
        Vector3D y,
        GalerkinElement receiverElement,
        GalerkinElement sourceElement,
        ArrayList<Geometry> shadowGeometryList,
        boolean isSceneGeometry,
        boolean isClusteredGeometry,
        GalerkinState galerkinState)
    {
        Ray ray = new Ray();
        ray.position.copy(y);
        ray.direction.subtraction(x, y);
        double distance = ray.direction.norm();
        if ( distance < Numeric.EPSILON ) {
            return 0.0;
        }
        ray.direction.scaledCopy((float)(1.0 / distance), ray.direction);

        double cosThetaY;
        if ( sourceElement.isCluster() ) {
            cosThetaY = 0.25;
        }
        else {
            cosThetaY = ray.direction.dotProduct(sourceElement.patch.normal);
            if ( cosThetaY <= 0.0 ) {
                return 0.0;
            }
        }

        double cosThetaX;
        if ( receiverElement.isCluster() ) {
            cosThetaX = 0.25;
        }
        else {
            cosThetaX = -ray.direction.dotProduct(receiverElement.patch.normal);
            if ( cosThetaX <= 0.0 ) {
                return 0.0;
            }
        }

        double formFactorKernelTerm = cosThetaX * cosThetaY / (Math.PI * distance * distance);
        float shortenedDistance = (float)(distance * (1.0 - Numeric.EPSILON));

        double visibilityFactor;
        if ( shadowGeometryList == null || shadowGeometryList.isEmpty() ) {
            visibilityFactor = 1.0;
        }
        else if ( galerkinState.multiResolutionVisibility == 0 ) {
            RayHit hitStore = new RayHit();
            RayHit h = shadowTestDiscretization(
                ray,
                shadowGeometryList,
                sceneWorldVoxelGrid,
                shadowCache,
                shortenedDistance,
                hitStore,
                isSceneGeometry,
                isClusteredGeometry);
            visibilityFactor = (h == null) ? 1.0 : 0.0;
        }
        else if ( shadowCache.cacheHit(ray, new float[] {shortenedDistance}, new RayHit()) != null ) {
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
                minimumFeatureSize);
        }

        return formFactorKernelTerm * visibilityFactor;
    }

    public static void computeAreaToAreaFormFactorVisibility(
        VoxelGrid sceneWorldVoxelGrid,
        ArrayList<Geometry> geometryShadowList,
        boolean isSceneGeometry,
        boolean isClusteredGeometry,
        Interaction link,
        GalerkinState galerkinState)
    {
        formFactorLastReceived = null;
        formFactorLastSource = null;

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
                link.deltaK = new float[] {1.0f};
                link.numberOfReceiverCubaturePositions = 1;
                link.visibility = (byte)128;
                return;
            }
        }
        else if ( receiverElement == sourceElement ) {
            link.deltaK = new float[] {0.0f};
            link.numberOfReceiverCubaturePositions = 1;
            link.visibility = 0;
            return;
        }

        CubatureRule[] receiverCubatureRuleRef = new CubatureRule[1];
        CubatureRule[] sourceCubatureRuleRef = new CubatureRule[1];
        Vector3D[] x = new Vector3D[CubatureRule.MAXIMUM_NODES];
        Vector3D[] y = new Vector3D[CubatureRule.MAXIMUM_NODES];
        for ( int i = 0; i < CubatureRule.MAXIMUM_NODES; i++ ) {
            x[i] = new Vector3D();
            y[i] = new Vector3D();
        }

        determineNodes(receiverElement, GalerkinRole.RECEIVER, galerkinState, receiverCubatureRuleRef, x);
        determineNodes(sourceElement, GalerkinRole.SOURCE, galerkinState, sourceCubatureRuleRef, y);

        CubatureRule receiverCubatureRule = receiverCubatureRuleRef[0];
        CubatureRule sourceCubatureRule = sourceCubatureRuleRef[0];
        if ( receiverCubatureRule == null || sourceCubatureRule == null ) {
            link.deltaK = new float[] {0.0f};
            link.numberOfReceiverCubaturePositions = 1;
            link.visibility = 0;
            return;
        }

        ShadowCache shadowCache = new ShadowCache();
        Patch.dontIntersect4(
            receiverElement.isCluster() ? null : receiverElement.patch,
            (receiverElement.isCluster() || receiverElement.patch == null) ? null : receiverElement.patch.twin,
            sourceElement.isCluster() ? null : sourceElement.patch,
            (sourceElement.isCluster() || sourceElement.patch == null) ? null : sourceElement.patch.twin);
        Geometry.dontIntersect(
            receiverElement.isCluster() ? receiverElement.geometry : null,
            sourceElement.isCluster() ? sourceElement.geometry : null);

        double maximumKernelValue = 0.0;
        double[][] Gxy = new double[CubatureRule.MAXIMUM_NODES][CubatureRule.MAXIMUM_NODES];
        int visibilityCount = 0;
        for ( int r = 0; r < receiverCubatureRule.numberOfNodes; r++ ) {
            for ( int s = 0; s < sourceCubatureRule.numberOfNodes; s++ ) {
                Gxy[r][s] = evaluatePointsPairKernel(
                    shadowCache,
                    sceneWorldVoxelGrid,
                    x[r],
                    y[s],
                    receiverElement,
                    sourceElement,
                    geometryShadowList,
                    isSceneGeometry,
                    isClusteredGeometry,
                    galerkinState);
                if ( Gxy[r][s] > maximumKernelValue ) {
                    maximumKernelValue = Gxy[r][s];
                }
                if ( Math.abs(Gxy[r][s]) > Numeric.EPSILON ) {
                    visibilityCount++;
                }
            }
        }

        Patch.dontIntersect0();
        Geometry.dontIntersect(null, null);

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
            link.deltaK = new float[] {0.0f};
            link.numberOfReceiverCubaturePositions = 1;
        }

        link.visibility = (byte)((255.0 * visibilityCount)
            / (receiverCubatureRule.numberOfNodes * sourceCubatureRule.numberOfNodes));
        if ( galerkinState.exactVisibility != 0 && geometryShadowList != null && link.visibility == (byte)255 ) {
            link.visibility = (byte)254;
        }

        if ( (receiverElement.isCluster() || sourceElement.isCluster()) ) {
            link.deltaK = new float[] {(float)(maximumKernelValue * sourceElement.area)};
            link.numberOfReceiverCubaturePositions = 1;
        }
        formFactorLastReceived = receiverElement;
        formFactorLastSource = sourceElement;
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
        link.deltaK = new float[1];
        if ( sourceRadiance[0].isBlack() ) {
            double gAverage = link.K[0] / receiverElement.area;
            link.deltaK[0] = (float)(gMax - gAverage);
            if ( gAverage - gMin > link.deltaK[0] ) {
                link.deltaK[0] = (float)(gAverage - gMin);
            }
        }
        else {
            link.deltaK[0] = 0.0f;
            for ( int k = 0; k < receiverCubatureRule.numberOfNodes; k++ ) {
                deltaRadiance[k].divide(deltaRadiance[k], sourceRadiance[0]);
                double delta = Math.abs(deltaRadiance[k].maximumComponent());
                if ( delta > link.deltaK[0] ) {
                    link.deltaK[0] = (float)delta;
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
