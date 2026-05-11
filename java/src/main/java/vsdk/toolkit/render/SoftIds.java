/**
Software ID rendering: because hardware ID rendering is tricky
due to frame buffer formats, etc.
*/

package vsdk.toolkit.render;

/**
Software ID rendering: because hardware ID rendering is tricky due to frame buffer
formats, etc.
*/

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.render.sgl.SglConstants;
import vsdk.toolkit.render.sgl.SglContext;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.tonemap.ToneMap;
import vsdk.toolkit.tonemap.ToneMappingContext;

public class SoftIds {
    /**
Sets up a software rendering context and initialises transforms and
viewport for the current view.
*/
    public static SglContext setupSoftFrameBuffer(Camera camera) {
        SglContext sgl = new SglContext(camera.xSize, camera.ySize);
        sgl.sglDepthTesting(true);
        sgl.sglClipping(true);
        sgl.sglClear(0, SglConstants.SGL_MAXIMUM_Z);

        Matrix4x4 p = Matrix4x4.createPerspectiveMatrix(
            camera.fieldOfVision * 2.0f * (float)Math.PI / 180.0f,
            (float)camera.xSize / (float)camera.ySize,
            camera.near,
            camera.far);
        sgl.sglLoadMatrix(p);
        Matrix4x4 l = Matrix4x4.createLookAtMatrix(camera.eyePosition, camera.lookPosition, camera.upDirection);
        sgl.sglMultiplyMatrix(l);

        return sgl;
    }

    public static void softRenderPatch(
        Patch patch,
        Camera camera,
        RenderOptions renderOptions,
        SglContext sglContext)
    {
        if (patch == null || camera == null || renderOptions == null || sglContext == null) {
            return;
        }

        Vector3D[] vertices = new Vector3D[4];

        if (renderOptions.backfaceCulling &&
            patch.normal.dotProduct(camera.eyePosition) + patch.planeConstant < Numeric.EPSILON) {
            return;
        }

        vertices[0] = patch.vertex[0].point;
        vertices[1] = patch.vertex[1].point;
        vertices[2] = patch.vertex[2].point;
        if (patch.numberOfVertices > 3) {
            vertices[3] = patch.vertex[3].point;
        }

        sglContext.sglSetPatch(patch);
        sglContext.sglPolygon(patch.numberOfVertices, vertices);
    }

    public static void softRenderPatches(Scene scene, RenderOptions renderOptions, SglContext sglContext) {
        if (scene == null || renderOptions == null || sglContext == null) {
            return;
        }

        for (int i = 0; scene.patchList != null && i < scene.patchList.size(); i++) {
            SoftIds.softRenderPatch(scene.patchList.get(i), scene.camera, renderOptions, sglContext);
        }
    }

    /**
Software ID rendering

Patch ID rendering. Returns an array of size (*x)*(*y) containing the IDs of
the patches visible through each pixel or 0 if the background is visible through
the pixel. x is normally the width and y the height of the canvas window
*/
    public static long[] softRenderIds(long[] x, long[] y, Scene scene, RenderOptions renderOptions) {
        SglContext currentSglContext = SoftIds.setupSoftFrameBuffer(scene.camera);
        SoftIds.softRenderPatches(scene, renderOptions, currentSglContext);

        if (x != null && x.length > 0) {
            x[0] = currentSglContext.width;
        }
        if (y != null && y.length > 0) {
            y[0] = currentSglContext.height;
        }

        long[] ids = new long[currentSglContext.width * currentSglContext.height];
        System.arraycopy(currentSglContext.frameBuffer, 0, ids, 0, ids.length);

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
                    rgb[rowRgbStart + i].r,
                    rgb[rowRgbStart + i].g,
                    rgb[rowRgbStart + i].b);
                ToneMap.toneMappingGammaCorrection(corrected_rgb, toneMapOptions);
                int pixelOffset = rowStart + 4 * i;
                c[pixelOffset] = (byte)(int)(corrected_rgb.r * 255.0);
                c[pixelOffset + 1] = (byte)(int)(corrected_rgb.g * 255.0);
                c[pixelOffset + 2] = (byte)(int)(corrected_rgb.b * 255.0);
                c[pixelOffset + 3] = (byte)255; // alpha = 1.0
            }
        }
    }
}
