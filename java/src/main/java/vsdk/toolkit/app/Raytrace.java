package vsdk.toolkit.app;

import java.io.OutputStream;
import java.util.Locale;
import vsdk.toolkit.app.options.OptionsGroupRaytracing;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.io.image.ImageOutputHandle;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.BidirectionalPathRaytracer;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.BidirectionalPathTracingState;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.LightList;
import vsdk.toolkit.raycasting.common.RayTracer;
import vsdk.toolkit.raycasting.simple.RayCaster;
import vsdk.toolkit.raycasting.simple.RayMatter;
import vsdk.toolkit.raycasting.simple.RayMatterState;
import vsdk.toolkit.raycasting.stochasticRaytracing.StochasticRaytracer;
import vsdk.toolkit.raycasting.stochasticRaytracing.StochasticRayTracingState;
import vsdk.toolkit.render.Canvas;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.tonemap.ToneMappingContext;

public final class Raytrace {
    private Raytrace() {
    }

    /**
This routine sets the current raytracing method to be used
*/
    private static void rayTraceSetMethod(
        RayTracer rayTracer,
        java.util.ArrayList<vsdk.toolkit.environment.geometry.elements.Patch> lightSourcePatches,
        LightList[] lightList)
    {
        if ( lightList != null ) {
            // Parameter intentionally unused in C++ implementation.
        }
        if ( rayTracer != null ) {
            rayTracer.initialize(lightSourcePatches);
        }
    }

    private static RayTracer rayTraceCreateRayTracerFromName(
        String rayTracerName,
        Scene scene,
        ToneMappingContext toneMapOptions,
        RayMatterState rayMatterState,
        BidirectionalPathTracingState bidirectionalPathState,
        StochasticRayTracingState stochasticRayTracingState,
        LightList[] lightList)
    {
        RayTracer newRaytracer;
        if ( "RayMatting".equals(rayTracerName) ) {
            newRaytracer = new RayMatter(null, scene.camera, rayMatterState, toneMapOptions);
        }
        else if ( "RayCasting".equals(rayTracerName) ) {
            newRaytracer = new RayCaster(null, scene.camera, toneMapOptions);
        }
        else if ( "BidirectionalPathTracing".equals(rayTracerName) ) {
            newRaytracer = new BidirectionalPathRaytracer(bidirectionalPathState, lightList == null ? null : lightList[0]);
        }
        else if ( "StochasticRaytracing".equals(rayTracerName) ) {
            newRaytracer = new StochasticRaytracer(lightList == null ? null : lightList[0], stochasticRayTracingState);
        }
        else {
            newRaytracer = null;
        }

        Raytrace.rayTraceSetMethod(newRaytracer, scene.lightSourcePatchList, lightList);

        if ( newRaytracer == null
             && (rayTracerName == null
                 || !rayTracerName.regionMatches(true, 0, "none", 0, 4)) ) {
            Logger.error(null, "Invalid raytracing method name '%s'", rayTracerName);
        }

        return newRaytracer;
    }

    public static RayTracer rayTraceCreate(
        Scene scene,
        ToneMappingContext toneMapOptions,
        String rayTracerName,
        RayMatterState rayMatterState,
        BidirectionalPathTracingState bidirectionalPathState,
        StochasticRayTracingState stochasticRayTracingState,
        LightList[] lightList)
    {
        RayTracer rayTracer = Raytrace.rayTraceCreateRayTracerFromName(
            rayTracerName,
            scene,
            toneMapOptions,
            rayMatterState,
            bidirectionalPathState,
            stochasticRayTracingState,
            lightList);

        if ( rayTracer != null ) {
            rayTracer.defaults();
        }
        return rayTracer;
    }

    public static void rayTraceSaveImage(
        String fileName,
        OutputStream stream,
        int isPipe,
        Scene scene,
        RayTracer rayTracer)
    {
        if ( stream == null ) {
            return;
        }

        long t = System.nanoTime();

        ImageOutputHandle img = ImageOutputHandle.createRadianceImageOutputHandle(
            fileName,
            stream,
            isPipe,
            scene.camera.xSize,
            scene.camera.ySize);
        if ( img == null ) {
            return;
        }

        if ( rayTracer == null ) {
            Logger.warning(null, "No ray tracing method active");
        }
        else if ( !rayTracer.saveImage(img) ) {
            Logger.warning(null, "No previous %s image available", rayTracer.getName());
        }

        ImageOutputHandle.deleteImageOutputHandle(img);

        System.out.printf(
            Locale.US,
            "Raytrace save image: %g secs.\n",
            (double)(System.nanoTime() - t) / 1000000000.0);
    }

    public static void rayTraceParseOptions(
        int[] argc,
        String[] argv,
        String[] rayTracerName)
    {
        OptionsGroupRaytracing.parse(argc, argv, rayTracerName);
    }

    public static void rayTraceExecute(
        String fileName,
        OutputStream stream,
        int isPipe,
        Scene scene,
        RadianceMethod radianceMethod,
        RayTracer rayTracer,
        ToneMappingContext toneMapOptions,
        RenderOptions renderOptions)
    {
        renderOptions.renderRayTracedImage = true;
        scene.camera.changed = 0;

        Canvas.canvasPushMode();
        RayTracer.rayTrace(
            fileName,
            stream,
            isPipe,
            rayTracer,
            scene,
            radianceMethod,
            toneMapOptions,
            renderOptions);
        Canvas.canvasPullMode();
    }
}
