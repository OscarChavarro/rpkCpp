package vsdk.toolkit.galerkin.processing;

import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.Interaction;
import vsdk.toolkit.galerkin.ShadowCache;
import vsdk.toolkit.numericalAnalysis.CubatureRule;
import vsdk.toolkit.skin.Geometry;

public class FormFactorClusteredStrategy {
    public static void doConstantAreaToAreaFormFactor(
        Interaction link,
        CubatureRule cubatureRuleRcv,
        CubatureRule cubatureRuleSrc,
        double[][] Gxy)
    {
        GalerkinElement receiverElement = link.receiverElement;
        GalerkinElement sourceElement = link.sourceElement;
        double Gx;
        double G = 0.0;
        double gMin = Numeric.HUGE_DOUBLE_VALUE;
        double gMax = -Numeric.HUGE_DOUBLE_VALUE;

        for ( int k = 0; k < cubatureRuleRcv.numberOfNodes; k++ ) {
            Gx = 0.0;
            for ( int l = 0; l < cubatureRuleSrc.numberOfNodes; l++ ) {
                Gx += cubatureRuleSrc.w[l] * Gxy[k][l];
            }
            Gx *= sourceElement.area;

            G += cubatureRuleRcv.w[k] * Gx;

            if ( Gx > gMax ) {
                gMax = Gx;
            }
            if ( Gx < gMin ) {
                gMin = Gx;
            }
        }
        link.K[0] = (float)(receiverElement.area * G);

        link.deltaK = (float)(G - gMin);
        if ( gMax - G > link.deltaK ) {
            link.deltaK = (float)(gMax - G);
        }
        link.numberOfReceiverCubaturePositions = 1;
    }

    public static double geomListMultiResolutionVisibility(
        ArrayList<Geometry> geometryOccluderList,
        ShadowCache shadowCache,
        Ray ray,
        float rcvDist,
        float srcSize,
        float minimumFeatureSize)
    {
        return 1.0;
    }

    public static double geometryMultiResolutionVisibility(
        ShadowCache shadowCache,
        Geometry geometry,
        Ray ray,
        float rcvDist,
        float srcSize,
        float minimumFeatureSize)
    {
        return 1.0;
    }
}
