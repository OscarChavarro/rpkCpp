package vsdk.toolkit.galerkin.processing;

import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Ray;
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
