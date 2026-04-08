package vsdk.toolkit.app;

import java.io.OutputStream;
import java.util.Locale;
import vsdk.toolkit.app.options.BatchOptions;
import vsdk.toolkit.app.options.OptionsGroupBatch;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.io.image.ImageOutputHandle;
import vsdk.toolkit.io.wrapper.FileUncompressWrapper;
import vsdk.toolkit.raycasting.common.RayTracer;
import vsdk.toolkit.render.Canvas;
import vsdk.toolkit.render.RadianceImageExporter;
import vsdk.toolkit.render.jogl.RenderOpenGL;
import vsdk.toolkit.galerkin.GalerkinRadianceMethod;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.tonemap.ToneMappingContext;

public final class Batch {
    @FunctionalInterface
    private interface ProcessFileCallback {
        void apply(
            String fileName,
            OutputStream outputStream,
            int isPipe,
            Scene scene,
            RadianceMethod radianceMethod,
            RayTracer rayTracer,
            ToneMappingContext toneMapOptions,
            RenderOptions renderOptions);
    }

    private static final BatchOptions batchOptions = new BatchOptions();
    private static RayTracer currentRayTracer = null;

    private Batch() {
    }

    public static BatchOptions batchGetOptions() {
        return batchOptions;
    }

    public static void generalParseOptions(int[] argc, String[] argv) {
        OptionsGroupBatch.parse(argc, argv, batchOptions);
    }

    private static void batchRayTraceSaveImage(
        String fileName,
        OutputStream outputStream,
        int isPipe,
        Scene scene,
        RadianceMethod radianceMethod,
        RayTracer rayTracer,
        ToneMappingContext toneMapOptions,
        RenderOptions renderOptions)
    {
        if ( radianceMethod != null || toneMapOptions != null || renderOptions != null ) {
            // Parameters intentionally unused in C++ implementation.
        }
        Raytrace.rayTraceSaveImage(fileName, outputStream, isPipe, scene, rayTracer);
    }

    /**
This routine was copied from uit.c, leaving out all interface related things
*/
    private static void batchProcessFile(
        String fileName,
        ProcessFileCallback processFileCallback,
        Scene scene,
        RadianceMethod radianceMethod,
        RayTracer rayTracer,
        ToneMappingContext toneMapOptions,
        RenderOptions renderOptions)
    {
        int[] isPipe = new int[] {0};
        OutputStream outputStream = FileUncompressWrapper.openOutputStreamCompressWrapper(fileName, isPipe);

        // Call the user supplied procedure to process the file
        processFileCallback.apply(
            fileName,
            outputStream,
            isPipe[0],
            scene,
            radianceMethod,
            rayTracer,
            toneMapOptions,
            renderOptions);

        FileUncompressWrapper.closeOutputStream(outputStream);
    }

    private static void batchSaveRadianceImage(
        String fileName,
        OutputStream outputStream,
        int isPipe,
        Scene scene,
        RadianceMethod radianceMethod,
        RayTracer rayTracer,
        ToneMappingContext toneMapOptions,
        RenderOptions renderOptions)
    {
        if ( rayTracer != null ) {
            // Parameter intentionally unused in C++ implementation.
        }

        if ( outputStream == null ) {
            return;
        }

        Canvas.canvasPushMode();

        String extension = ImageOutputHandle.imageFileExtension(fileName);
        if ( extension != null && extension.regionMatches(true, 0, "logluv", 0, 6) ) {
            System.out.printf("Saving LOGLUV image to file '%s' ....... ", fileName);
        }
        else {
            System.out.printf("Saving RGB image to file '%s' .......... ", fileName);
        }
        System.out.flush();

        long t = System.nanoTime();

        RadianceImageExporter.exportImage(
            fileName,
            outputStream,
            isPipe,
            scene,
            radianceMethod,
            toneMapOptions,
            renderOptions);

        System.out.printf(Locale.US, "%g secs.\n", (double)(System.nanoTime() - t) / 1000000000.0);
        Canvas.canvasPullMode();
    }

    private static void batchSaveRadianceModel(
        String fileName,
        OutputStream outputStream,
        int isPipe,
        Scene scene,
        RadianceMethod radianceMethod,
        RayTracer rayTracer,
        ToneMappingContext toneMapOptions,
        RenderOptions renderOptions)
    {
        if ( isPipe != 0 || rayTracer != null || toneMapOptions != null ) {
            // Parameters intentionally unused in C++ implementation.
        }

        if ( outputStream == null ) {
            return;
        }

        Canvas.canvasPushMode();
        System.out.printf("Saving VRML model to file '%s' ... ", fileName);
        System.out.flush();
        long t = System.nanoTime();

        if ( radianceMethod != null ) {
            radianceMethod.writeVRML(scene.camera, outputStream, renderOptions);
        }

        System.out.printf(Locale.US, "%g secs.\n", (double)(System.nanoTime() - t) / 1000000000.0);
        Canvas.canvasPullMode();
    }

    private static String formatFileName(String format, int iterationNumber) {
        if ( format == null || format.isEmpty() ) {
            return "";
        }
        try {
            return String.format(Locale.US, format, iterationNumber);
        }
        catch ( Exception ignored ) {
            return format;
        }
    }

    public static void batchExecuteRadianceSimulation(
        Scene scene,
        RadianceMethod radianceMethod,
        RayTracer rayTracer,
        ToneMappingContext toneMapOptions,
        RenderOptions renderOptions)
    {
        if ( scene == null || scene.geometryList == null || scene.geometryList.size() == 0 ) {
            System.out.printf("Empty world? Missing argument to some command line parameter option?\n");
            return;
        }

        Batch.currentRayTracer = rayTracer;

        long startTime = System.nanoTime();
        float wastedSecs = 0.0f;

        if ( radianceMethod != null ) {
            System.out.printf("Doing %s ...\n", radianceMethod.getRadianceMethodName());

            System.out.flush();
            System.err.flush();

            boolean done = false;
            for ( int iterationNumber = 0;
                  iterationNumber < batchOptions.iterations && !done;
                  iterationNumber++ ) {
                System.out.printf(
                    "-----------------------------------\n"
                    + "radiance iteration %04d\n"
                    + "-----------------------------------\n\n",
                    iterationNumber);

                Canvas.canvasPushMode();
                done = radianceMethod.doStep(scene, renderOptions);
                Canvas.canvasPullMode();

                System.out.flush();
                System.err.flush();

                System.out.printf("%s", radianceMethod.getStats());

		RenderOpenGL.renderGetNearFar(scene.camera, scene.geometryList);

                System.out.flush();
                System.err.flush();

                long wastedStart = System.nanoTime();

                if ( batchOptions.saveModulo != 0
                     && (iterationNumber % batchOptions.saveModulo) == 0
                     && batchOptions.radianceImageFileNameFormat != null
                     && !batchOptions.radianceImageFileNameFormat.isEmpty() ) {
                    String fileName = formatFileName(batchOptions.radianceImageFileNameFormat, iterationNumber);
                    Batch.batchProcessFile(
                        fileName,
                        Batch::batchSaveRadianceImage,
                        scene,
                        radianceMethod,
                        rayTracer,
                        toneMapOptions,
                        renderOptions);
                }

                if ( batchOptions.radianceModelFileNameFormat != null
                     && !batchOptions.radianceModelFileNameFormat.isEmpty() ) {
                    String fileName = formatFileName(batchOptions.radianceModelFileNameFormat, iterationNumber);
                    Batch.batchProcessFile(
                        fileName,
                        Batch::batchSaveRadianceModel,
                        scene,
                        radianceMethod,
                        rayTracer,
                        toneMapOptions,
                        renderOptions);
                }

                wastedSecs += (float)((double)(wastedStart - System.nanoTime()) / 1000000000.0);

                System.out.flush();
                System.err.flush();
            }
        }
        else {
            System.out.printf("(No world-space radiance computations are being done)\n");
        }

        if ( batchOptions.timings != 0 ) {
            System.out.printf(
                Locale.US,
                "Radiance total time %g secs.\n",
                (double)(System.nanoTime() - startTime) / 1000000000.0 - wastedSecs);
        }

        if ( Batch.currentRayTracer != null ) {
            System.out.printf("Doing %s ...\n", Batch.currentRayTracer.getName());

            startTime = System.nanoTime();
            Raytrace.rayTraceExecute(
                null,
                null,
                0,
                scene,
                radianceMethod,
                Batch.currentRayTracer,
                toneMapOptions,
                renderOptions);

            if ( batchOptions.timings != 0 ) {
                System.out.printf(
                    Locale.US,
                    "Raytracing total time %g secs.\n",
                    (double)(System.nanoTime() - startTime) / 1000000000.0);
            }

            Batch.batchProcessFile(
                batchOptions.raytracingImageFileName,
                Batch::batchRayTraceSaveImage,
                scene,
                radianceMethod,
                Batch.currentRayTracer,
                toneMapOptions,
                renderOptions);
        }
        else {
            System.out.printf("(No pixel-based radiance computations are being done)\n");
        }

        System.out.printf("Computations finished.\n");
        Batch.currentRayTracer = null;
    }
}
