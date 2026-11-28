/**
Software ID rendering: because hardware ID rendering is tricky
due to frame buffer formats, etc.
*/

package vsdk.toolkit.render;

/**
Software ID rendering: because hardware ID rendering is tricky due to frame buffer
formats, etc.
*/

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.tonemap.ToneMap;
import vsdk.toolkit.tonemap.ToneMappingContext;

public class SoftIds {
    private static void drawPatch(
        Patch patch,
        Camera camera,
        RendererConfiguration renderOptions,
        FaceVisibilityBuffer target)
    {
        if ( renderOptions.backfaceCulling &&
             patch.getNormal().dotProduct(camera.eyePosition) + patch.getPlaneConstant() < VSDK.EPSILON ) {
            return;
        }

        Vector3Dd[] vertices = new Vector3Dd[4];
        final int count = patch.getNumberOfVertices();
        for ( int i = 0; i < count; i++ ) {
            vertices[i] = patch.getVertices()[i].point;
        }
        target.drawPolygon(count, vertices, new FaceIndexSample(patch.getFaceIndex()));
    }

    /** Renders into a cleared target with the camera's dimensions and full viewport. */
    public static void renderVisibleFaces(Scene scene, RendererConfiguration renderOptions, FaceVisibilityBuffer target) {
        if ( renderOptions == null ) {
            return;
        }

        final Camera camera = scene.camera;
        final Matrix4x4d projection = Camera.perspectiveMatrix(
            camera.fieldOfVision * 2.0 * (double)Math.PI / 180.0,
            (double)camera.xSize / (double)camera.ySize,
            camera.near,
            camera.far);
        final Matrix4x4d view = Camera.lookAtMatrix(camera.eyePosition, camera.lookPosition, camera.upDirection);
        target.setTransform(projection.multiply(view));

        for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
            final Patch patch = scene.patchList.get(i);
            if ( patch != null ) {
                drawPatch(patch, camera, renderOptions, target);
            }
        }
    }

    /**
Legacy patch ID export. Zero represents the background.
*/
    public static long[] softRenderIds(long[] width, long[] height, Scene scene, RendererConfiguration renderOptions) {
        FaceVisibilityBuffer target = new FaceVisibilityBuffer(scene.camera.xSize, scene.camera.ySize, FaceIndexSample.background());
        renderVisibleFaces(scene, renderOptions, target);

        width[0] = target.getWidth();
        height[0] = target.getHeight();
        long[] ids = new long[(int)(width[0] * height[0])];
        for ( int y = 0; y < height[0]; y++ ) {
            final int rowStart = (int)(y * width[0]);
            for ( int x = 0; x < width[0]; x++ ) {
                final FaceIndexSample face = target.getSample(x, y);
                ids[rowStart + x] = face.isBackground() ? 0 : scene.getFace(face.getIndex()).getId();
            }
        }
        return ids;
    }

    /**
Renders in memory an image of m lines of n pixels at column x on row y (= lower
left corner of image, relative to the lower left corner of the window)
*/
    public static void softRenderPixels(int width, int height, ColorRgb[] rgb, ToneMappingContext toneMapOptions) {
        // Length of one row of RGBA image data rounded up to a multiple of 8
        int rowLength = (4 * width * Byte.BYTES + 7) & ~7;
        byte[] c = new byte[height * rowLength + 8];

        for (int j = 0; j < height; j++) {
            int rowRgbStart = j * width;
            int rowStart = j * rowLength;
            for (int i = 0; i < width; i++) {
                ColorRgb corrected_rgb = new ColorRgb(
                    rgb[rowRgbStart + i].getR(),
                    rgb[rowRgbStart + i].getG(),
                    rgb[rowRgbStart + i].getB());
                corrected_rgb = ToneMap.toneMappingGammaCorrection(corrected_rgb, toneMapOptions);
                int pixelOffset = rowStart + 4 * i;
                c[pixelOffset] = (byte)(int)(corrected_rgb.getR() * 255.0);
                c[pixelOffset + 1] = (byte)(int)(corrected_rgb.getG() * 255.0);
                c[pixelOffset + 2] = (byte)(int)(corrected_rgb.getB() * 255.0);
                c[pixelOffset + 3] = (byte)255; // alpha = 1.0
            }
        }
    }
}
