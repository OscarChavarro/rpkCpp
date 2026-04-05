package vsdk.toolkit.render;

import java.io.OutputStream;

import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.io.image.ImageOutputHandle;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.tonemap.ToneMappingContext;

public final class RadianceImageExporter {
    public static void exportImage(
        String fileName,
        OutputStream outputStream,
        int isPipe,
        Scene scene,
        RadianceMethod radianceMethod,
        ToneMappingContext toneMapOptions,
        RenderOptions renderOptions)
    {
        if (outputStream == null || scene == null || scene.camera == null) {
            return;
        }

        if (toneMapOptions == null) {
            Error.error("RadianceImageExporter::exportImage", "Tone mapping context not provided for image export");
            return;
        }

        ScreenBuffer screenBuffer = new ScreenBuffer(null, scene.camera, toneMapOptions);
        SoftIdsWrapper idRenderer = new SoftIdsWrapper(scene, renderOptions);

        long[] width = new long[1];
        long[] height = new long[1];
        idRenderer.getSize(width, height);
        if (width[0] != screenBuffer.getHRes() || height[0] != screenBuffer.getVRes()) {
            Error.error("RadianceImageExporter::exportImage", "ID buffer size does not match screen size");
            return;
        }

        for (int y = 0; y < height[0]; y++) {
            for (int x = 0; x < width[0]; x++) {
                Patch patch = idRenderer.getPatchAtPixel(x, y);
                if (patch != null) {
                    ColorRgb radiance = RadianceImageExporter.getRadianceAtPixel(
                        screenBuffer,
                        scene.camera,
                        x,
                        y,
                        patch,
                        radianceMethod,
                        renderOptions);
                    screenBuffer.add(x, y, radiance);
                }
            }
        }
        // Temporary diagnostics for Java/C++ parity verification.
        int coveredPixels = 0;
        int nonBlackPixels = 0;
        for (int y = 0; y < height[0]; y++) {
            for (int x = 0; x < width[0]; x++) {
                Patch patch = idRenderer.getPatchAtPixel(x, y);
                if (patch != null) {
                    coveredPixels++;
                    ColorRgb sample = screenBuffer.get(x, y);
                    if (!sample.isBlack()) {
                        nonBlackPixels++;
                    }
                }
            }
        }
        System.err.printf(
            "RadianceImageExporter: coveredPixels=%d nonBlackPixels=%d (%dx%d)\n",
            coveredPixels, nonBlackPixels, width[0], height[0]);

        ImageOutputHandle imageOutputHandle = ImageOutputHandle.createRadianceImageOutputHandle(
            fileName,
            outputStream,
            isPipe,
            scene.camera.xSize,
            scene.camera.ySize);
        if (imageOutputHandle == null) {
            return;
        }

        screenBuffer.writeFile(imageOutputHandle);
        ImageOutputHandle.deleteImageOutputHandle(imageOutputHandle);
    }

    private static void clipUv(int numberOfVertices, double[] u, double[] v) {
        if (u[0] > 1.0 - Numeric.EPSILON) {
            u[0] = 1.0 - Numeric.EPSILON;
        }
        if (v[0] > 1.0 - Numeric.EPSILON) {
            v[0] = 1.0 - Numeric.EPSILON;
        }
        if (numberOfVertices == 3 && (u[0] + v[0]) > 1.0 - Numeric.EPSILON) {
            if (u[0] > v[0]) {
                u[0] = 1.0 - v[0] - Numeric.EPSILON;
            }
            else {
                v[0] = 1.0 - u[0] - Numeric.EPSILON;
            }
        }
        if (u[0] < Numeric.EPSILON) {
            u[0] = Numeric.EPSILON;
        }
        if (v[0] < Numeric.EPSILON) {
            v[0] = Numeric.EPSILON;
        }
    }

    private static ColorRgb getRadianceAtPixel(
        ScreenBuffer screenBuffer,
        Camera camera,
        int x,
        int y,
        Patch patch,
        RadianceMethod radianceMethod,
        RenderOptions renderOptions)
    {
        ColorRgb radiance = new ColorRgb();
        radiance.clear();

        if (screenBuffer == null || camera == null || patch == null || radianceMethod == null || renderOptions == null) {
            return radiance;
        }

        Vector3D rayDirection = screenBuffer.getPixelVector(x, y);
        rayDirection.normalize(Numeric.EPSILON_FLOAT);

        float denominator = patch.normal.dotProduct(rayDirection);
        if (denominator <= Numeric.EPSILON_FLOAT && denominator >= -Numeric.EPSILON_FLOAT) {
            return radiance;
        }

        float distance =
            -(patch.normal.dotProduct(camera.eyePosition) + patch.planeConstant) / denominator;
        Vector3D hitPoint = new Vector3D();
        hitPoint.sumScaled(camera.eyePosition, distance, rayDirection);

        double[] u = new double[1];
        double[] v = new double[1];
        patch.uv(hitPoint, u, v);
        RadianceImageExporter.clipUv(patch.numberOfVertices, u, v);

        Vector3D eyeDirection = new Vector3D(-rayDirection.x, -rayDirection.y, -rayDirection.z);
        return radianceMethod.getRadiance(camera, patch, u[0], v[0], eyeDirection, renderOptions);
    }
}
