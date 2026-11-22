package vsdk.toolkit.render;

import java.io.OutputStream;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.io.image.ImageOutputHandle;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.environment.geometry.elements.Patch;
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
            Logger.reportMessage(null, Logger.ERROR, "RadianceImageExporter::exportImage", "Tone mapping context not provided for image export");
            return;
        }

        ScreenBuffer screenBuffer = new ScreenBuffer(null, scene.camera, toneMapOptions);
        SoftIdsWrapper idRenderer = new SoftIdsWrapper(scene, renderOptions);

        long[] width = new long[1];
        long[] height = new long[1];
        idRenderer.getSize(width, height);
        if (width[0] != screenBuffer.getHRes() || height[0] != screenBuffer.getVRes()) {
            Logger.reportMessage(null, Logger.ERROR, "RadianceImageExporter::exportImage", "ID buffer size does not match screen size");
            return;
        }

        for (int y = 0; y < height[0]; y++) {
            for (int x = 0; x < width[0]; x++) {
                Patch patch = idRenderer.getPatchAtPixel(x, y);
                if (patch != null) {
                    ColorRgbMutable radiance = RadianceImageExporter.getRadianceAtPixel(
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
        if (u[0] > 1.0 - VSDK.EPSILON) {
            u[0] = 1.0 - VSDK.EPSILON;
        }
        if (v[0] > 1.0 - VSDK.EPSILON) {
            v[0] = 1.0 - VSDK.EPSILON;
        }
        if (numberOfVertices == 3 && (u[0] + v[0]) > 1.0 - VSDK.EPSILON) {
            if (u[0] > v[0]) {
                u[0] = 1.0 - v[0] - VSDK.EPSILON;
            }
            else {
                v[0] = 1.0 - u[0] - VSDK.EPSILON;
            }
        }
        if (u[0] < VSDK.EPSILON) {
            u[0] = VSDK.EPSILON;
        }
        if (v[0] < VSDK.EPSILON) {
            v[0] = VSDK.EPSILON;
        }
    }

    private static ColorRgbMutable getRadianceAtPixel(
        ScreenBuffer screenBuffer,
        Camera camera,
        int x,
        int y,
        Patch patch,
        RadianceMethod radianceMethod,
        RenderOptions renderOptions)
    {
        ColorRgbMutable radiance = new ColorRgbMutable();
        radiance.clear();

        if (screenBuffer == null || camera == null || patch == null || radianceMethod == null || renderOptions == null) {
            return radiance;
        }

        Vector3Df rayDirection = screenBuffer.getPixelVector(x, y);
        rayDirection = rayDirection.normalized(Numeric.EPSILON_FLOAT);

        float denominator = patch.getNormal().dotProduct(rayDirection);
        if (denominator <= Numeric.EPSILON_FLOAT && denominator >= -Numeric.EPSILON_FLOAT) {
            return radiance;
        }

        float distance =
            -(patch.getNormal().dotProduct(camera.eyePosition) + patch.planeConstant) / denominator;
        Vector3Df hitPoint = new Vector3Df();
        hitPoint = camera.eyePosition.add(rayDirection.multiply(distance));

        double[] u = new double[1];
        double[] v = new double[1];
        patch.uv(hitPoint, u, v);
        RadianceImageExporter.clipUv(patch.numberOfVertices, u, v);

        Vector3Df eyeDirection = new Vector3Df(-rayDirection.x(), -rayDirection.y(), -rayDirection.z());
        return radianceMethod.getRadiance(camera, patch, u[0], v[0], eyeDirection, renderOptions);
    }
}
