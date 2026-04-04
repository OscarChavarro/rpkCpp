package vsdk.toolkit.app;

import vsdk.toolkit.app.options.OptionsGroupRadiance;
import vsdk.toolkit.galerkin.GalerkinRadianceMethod;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.BidirectionalPathTracingState;
import vsdk.toolkit.raycasting.photonMap.PhotonMapConfig;
import vsdk.toolkit.raycasting.photonMap.PhotonMapState;
import vsdk.toolkit.raycasting.simple.RayMatterState;
import vsdk.toolkit.raycasting.stochasticRaytracing.ElementHierarchyState;
import vsdk.toolkit.raycasting.stochasticRaytracing.StochasticRadiosityBasisState;
import vsdk.toolkit.raycasting.stochasticRaytracing.StochasticRayTracingState;
import vsdk.toolkit.raycasting.stochasticRaytracing.StochasticRelaxation;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.RadianceMethodAlgorithm;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.tonemap.ToneMappingContext;

/**
Stuff common to all radiance methods
*/
public final class Radiance {
    private Radiance() {
    }

    /**
This routine sets the current radiance method to be used + initializes
*/
    public static void setRadianceMethod(
        RadianceMethod radianceMethod,
        Scene scene,
        ToneMappingContext toneMapOptions)
    {
        if ( radianceMethod != null ) {
            radianceMethod.terminate(scene.patchList);
            // Until we have radiance data convertors, we dispose of the old data and
            // allocate new data for the new method
            for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
                Patch patch = scene.patchList.get(i);
                if ( patch != null ) {
                    radianceMethod.destroyPatchData(patch);
                }
            }
            for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
                Patch patch = scene.patchList.get(i);
                if ( patch != null ) {
                    patch.radianceData = radianceMethod.createPatchData(patch);
                }
            }
            radianceMethod.initialize(scene, toneMapOptions);
        }
    }

    /**
Parses (and consumes) command line options for radiance
computation
*/
    public static void radianceParseOptions(
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
        OptionsGroupRadiance.parse(
            argc,
            argv,
            newRadianceMethod,
            stochasticRelaxationState,
            elementHierarchyState,
            stochasticRadiosityBasisState,
            photonMapState,
            photonMapConfig,
            rayMatterState,
            bidirectionalPathState,
            stochasticRayTracingState);

        if ( newRadianceMethod != null
             && newRadianceMethod.length > 0
             && newRadianceMethod[0] != null ) {
            if ( newRadianceMethod[0].className == RadianceMethodAlgorithm.GALERKIN ) {
                GalerkinRadianceMethod galerkinRadianceMethod = (GalerkinRadianceMethod)newRadianceMethod[0];
                galerkinRadianceMethod.setStrategy();
            }
            newRadianceMethod[0].parseOptions(argc, argv);
        }
    }
}
