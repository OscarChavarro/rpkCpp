package vsdk.toolkit.render;

import java.io.OutputStream;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.skin.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
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
        RendererConfiguration renderOptions)
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
        ImageOutputTarget imageOutputTarget = ImageOutputTarget.create(fileName, outputStream, isPipe);
        if (imageOutputTarget == null) {
            return;
        }

        screenBuffer.writeFile(imageOutputTarget);
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

    private static ColorRgb getRadianceAtPixel(
        ScreenBuffer screenBuffer,
        Camera camera,
        int x,
        int y,
        Patch patch,
        RadianceMethod radianceMethod,
        RendererConfiguration renderOptions)
    {
        ColorRgb radiance = new ColorRgb();
        radiance = new ColorRgb(0.0, 0.0, 0.0);

        if (screenBuffer == null || camera == null || patch == null || radianceMethod == null || renderOptions == null) {
            return radiance;
        }

        Vector3Dd rayDirection = screenBuffer.getPixelVector(x, y);
        rayDirection = rayDirection.normalized(Numeric.EPSILON);

        double denominator = patch.getNormal().dotProduct(rayDirection);
        if (denominator <= Numeric.EPSILON && denominator >= -Numeric.EPSILON) {
            return radiance;
        }

        double distance =
            -(patch.getNormal().dotProduct(camera.eyePosition) + patch.getPlaneConstant()) / denominator;
        Vector3Dd hitPoint = new Vector3Dd();
        hitPoint = camera.eyePosition.add(rayDirection.multiply(distance));

        double[] u = new double[1];
        double[] v = new double[1];
        patch.uv(hitPoint, u, v);
        RadianceImageExporter.clipUv(patch.getNumberOfVertices(), u, v);

        Vector3Dd eyeDirection = new Vector3Dd(-rayDirection.x(), -rayDirection.y(), -rayDirection.z());
        return radianceMethod.getRadiance(camera, patch, u[0], v[0], eyeDirection, renderOptions);
    }
}
