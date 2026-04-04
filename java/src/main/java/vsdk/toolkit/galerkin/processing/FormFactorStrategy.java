/**
All kind of form factor computations
*/

package vsdk.toolkit.galerkin.processing;

import java.util.ArrayList;
import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.galerkin.GalerkinBasis;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinRole;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.galerkin.Interaction;
import vsdk.toolkit.galerkin.ShadowCache;
import vsdk.toolkit.numericalAnalysis.CubatureRule;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.skin.Geometry;

public class FormFactorStrategy {
    // Global variables used for form factor computation optimisation
    private static GalerkinElement formFactorLastReceived;
    private static GalerkinElement formFactorLastSource;

    public static void computeAreaToAreaFormFactorVisibility(
        VoxelGrid sceneWorldVoxelGrid,
        ArrayList<Geometry> geometryShadowList,
        boolean isSceneGeometry,
        boolean isClusteredGeometry,
        Interaction link,
        GalerkinState galerkinState)
    {
    }

    @SuppressWarnings("unused")
    private static void determineNodes(
        GalerkinElement element,
        GalerkinRole role,
        GalerkinState galerkinState,
        CubatureRule[] cr,
        Vector3D[] x)
    {
    }

    @SuppressWarnings("unused")
    private static void doHigherOrderAreaToAreaFormFactor(
        Interaction twoPatchesInteraction,
        CubatureRule receiverCubatureRule,
        CubatureRule sourceCubatureRule,
        double[][] Gxy,
        GalerkinState galerkinState)
    {
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
    }

    @SuppressWarnings("unused")
    private static void computeInteractionFormFactor(
        CubatureRule receiverCubatureRule,
        CubatureRule sourceCubatureRule,
        double[][] Gxy,
        GalerkinElement sourceElement,
        GalerkinElement receiverElement,
        GalerkinBasis sourceBasis,
        GalerkinBasis receiverBasis,
        ColorRgb[] sourceRadiance,
        double[] gMin,
        double[] gMax,
        ColorRgb[] deltaRadiance,
        Interaction twoPatchesInteraction)
    {
    }
}
