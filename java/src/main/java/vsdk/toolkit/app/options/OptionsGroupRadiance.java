package vsdk.toolkit.app.options;

import vsdk.toolkit.galerkin.GalerkinRadianceMethod;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.BidirectionalPathTracingState;
import vsdk.toolkit.raycasting.photonMap.PhotonMapConfig;
import vsdk.toolkit.raycasting.photonMap.PhotonMapRadianceMethod;
import vsdk.toolkit.raycasting.photonMap.PhotonMapState;
import vsdk.toolkit.raycasting.simple.RayMatterState;
import vsdk.toolkit.raycasting.stochasticRaytracing.ElementHierarchyState;
import vsdk.toolkit.raycasting.stochasticRaytracing.RandomWalkRadianceMethod;
import vsdk.toolkit.raycasting.stochasticRaytracing.StochasticJacobiRadianceMethod;
import vsdk.toolkit.raycasting.stochasticRaytracing.StochasticRadiosityBasisState;
import vsdk.toolkit.raycasting.stochasticRaytracing.StochasticRayTracingState;
import vsdk.toolkit.raycasting.stochasticRaytracing.StochasticRelaxation;
import vsdk.toolkit.scene.RadianceMethod;

public final class OptionsGroupRadiance {
    private static final int RADIANCE_METHODS_STRING_LENGTH = 1000;

    private OptionsGroupRadiance() {
    }

    private static void selectRadianceMethod(
        String name,
        RadianceMethod[] newRadianceMethod,
        StochasticRelaxation stochasticRelaxationState,
        ElementHierarchyState elementHierarchyState,
        StochasticRadiosityBasisState stochasticRadiosityBasisState,
        PhotonMapState photonMapState,
        PhotonMapConfig photonMapConfig)
    {
        if ( name != null && !name.isEmpty() ) {
            if ( newRadianceMethod != null && newRadianceMethod.length > 0 && newRadianceMethod[0] != null ) {
                newRadianceMethod[0] = null;
            }

            if ( OptionTextUtils.equalsIgnoreCasePrefix(name, "Galerkin", 4) ) {
                newRadianceMethod[0] = new GalerkinRadianceMethod();
            }
            else if ( OptionTextUtils.equalsIgnoreCasePrefix(name, "PMAP", 4) ) {
                newRadianceMethod[0] = new PhotonMapRadianceMethod(photonMapState, photonMapConfig);
            }
            else if ( OptionTextUtils.equalsIgnoreCasePrefix(name, "StochJacobi", 4) ) {
                newRadianceMethod[0] = new StochasticJacobiRadianceMethod(
                    stochasticRelaxationState,
                    elementHierarchyState,
                    stochasticRadiosityBasisState);
            }
            else if ( OptionTextUtils.equalsIgnoreCasePrefix(name, "RandomWalk", 4) ) {
                newRadianceMethod[0] = new RandomWalkRadianceMethod(
                    stochasticRelaxationState,
                    elementHierarchyState,
                    stochasticRadiosityBasisState);
            }
        }
    }

    public static void parse(
        int[] argc,
        String[] argv,
        RadianceMethod[] newRadianceMethod,
        StochasticRelaxation stochasticRelaxationState,
        ElementHierarchyState elementHierarchyState,
        StochasticRadiosityBasisState stochasticRadiosityBasisState,
        PhotonMapState photonMapState,
        PhotonMapConfig photonMapConfig,
        RayMatterState rayMatterState,
        BidirectionalPathTracingState bidirectionalPathState,
        StochasticRayTracingState stochasticRayTracingState)
    {
        String[] radianceMethodsString = new String[] {""};
        if ( radianceMethodsString[0].length() > RADIANCE_METHODS_STRING_LENGTH ) {
            radianceMethodsString[0] = radianceMethodsString[0].substring(0, RADIANCE_METHODS_STRING_LENGTH);
        }

        OptionsGroupRadianceMethod.radianceMethodParseOptions(argc, argv, radianceMethodsString);

        OptionsGroupRadiance.selectRadianceMethod(
            radianceMethodsString[0],
            newRadianceMethod,
            stochasticRelaxationState,
            elementHierarchyState,
            stochasticRadiosityBasisState,
            photonMapState,
            photonMapConfig);

        if ( newRadianceMethod == null || newRadianceMethod.length == 0 || newRadianceMethod[0] == null ) {
            System.err.printf(
                "ERROR: You must select a radiance mode using '-radiance-method'. Supported values: Galerkin, PMAP, StochJacobi, RandomWalk.\n");
            System.err.flush();
            System.exit(1);
        }

        OptionsGroupStochasticRelaxationRadiosity.parse(argc, argv, stochasticRelaxationState, elementHierarchyState);
        OptionsGroupRandomWalkRadiosity.parse(argc, argv, stochasticRelaxationState);
        OptionsGroupRayMatter.rayMattingParseOptions(argc, argv, rayMatterState);
        OptionsGroupBidirectionalRaytracing.parse(argc, argv, bidirectionalPathState);
        OptionsGroupStochasticRaytracing.parse(argc, argv, stochasticRayTracingState);
        OptionsGroupPhotonMap.parse(argc, argv, photonMapState);

        OptionsGroupGalerkin.galerkinParseOptions(argc, argv);
    }
}
