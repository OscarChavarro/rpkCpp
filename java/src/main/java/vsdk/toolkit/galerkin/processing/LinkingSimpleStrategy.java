package vsdk.toolkit.galerkin.processing;

import java.util.ArrayList;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinRole;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.Patch;

public class LinkingSimpleStrategy {
    private static void createInitialLink(
        Scene scene,
        GalerkinState galerkinState,
        GalerkinRole role,
        ArrayList<Geometry>[] candidateList,
        GalerkinElement topElement,
        BoundingBox topLevelBoundingBox,
        Patch patch)
    {
    }

    private static void geometryLink(
        Scene scene,
        GalerkinState galerkinState,
        GalerkinRole role,
        ArrayList<Geometry>[] candidateList,
        GalerkinElement topElement,
        BoundingBox topLevelBoundingBox,
        Geometry geometry)
    {
    }

    public static void createInitialLinks(
        Scene scene,
        GalerkinState galerkinState,
        GalerkinRole role,
        GalerkinElement topElement)
    {
    }
}
