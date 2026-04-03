/**
Routines dealing with view potential
*/

package vsdk.toolkit.render;

import java.util.ArrayList;

import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.render.sgl.SglContext;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.skin.Patch;

/**
In analogy with [SMIT1992] Smits, "Importance-driven Radiosity", SIGGRAPH '92, we
call the integral of potential over surface area "importance"
*/
public class Potential {
    /**
Updates directly received potential for all patches
*/
    public static void updateDirectPotential(Scene scene, RenderOptions renderOptions) {
        Canvas.canvasPushMode();

        // Get the patch IDs for each pixel
        long[] x = new long[1];
        long[] y = new long[1];
        long[] ids = SoftIds.softRenderIds(x, y, scene, renderOptions);

        Canvas.canvasPullMode();

        if (ids == null) {
            return;
        }

        long lostPixels = 0;

        // Build a table to convert a patch ID to the corresponding Patch
        int maximumPatchId = Patch.getNextId() - 1;
        Patch[] id2patch = new Patch[maximumPatchId + 1];
        for (int i = 0; i <= maximumPatchId; i++) {
            id2patch[i] = null;
        }
        for (int i = 0; scene.patchList != null && i < scene.patchList.size(); i++) {
            Patch patch = scene.patchList.get(i);
            id2patch[patch.id] = patch;
        }

        // Allocate space for an array to hold the new direct potential of the patches
        float[] newDirectImportance = new float[maximumPatchId + 1];
        for (int i = 0; i <= maximumPatchId; i++) {
            newDirectImportance[i] = 0.0f;
        }

        // h and v are the horizontal resp. vertical distance between two
        // neighboring pixels on the screen
        float h = 2.0f * (float)Math.tan(scene.camera.horizontalFov * (float)Math.PI / 180.0f) / (float)x[0];
        float v = 2.0f * (float)Math.tan(scene.camera.verticalFov * (float)Math.PI / 180.0f) / (float)y[0];
        float pixelArea = h * v;

        float ySample;
        long j;
        for (j = y[0] - 1, ySample = -v * (float)(y[0] - 1) / 2.0f;
             j >= 0;
             j--, ySample += v) {
            long rowStart = j * x[0];
            float xSample = -h * (float)(x[0] - 1) / 2.0f;
            for (long i = 0; i < x[0]; i++, xSample += (long)h) {
                long the_id = ids[(int)(rowStart + i)] & 0xffffffL;

                if (the_id > 0 && the_id <= maximumPatchId) {
                    Vector3D pixDir = new Vector3D();

                    // Compute direction to center of pixel
                    pixDir.combine3(scene.camera.Z, (float)xSample, scene.camera.X, ySample, scene.camera.Y);

                    // Delta_importance = (cosine of the angle between the direction to
                    // the pixel and the viewing direction, over the distance from the
                    // eye point to the pixel) squared, times area of the pixel
                    float deltaImportance = scene.camera.Z.dotProduct(pixDir) / pixDir.dotProduct(pixDir);
                    deltaImportance *= deltaImportance * pixelArea;
                    newDirectImportance[(int)the_id] += deltaImportance;
                }
                else if (the_id > maximumPatchId) {
                    lostPixels++;
                }
            }
        }

        if (lostPixels > 0) {
            Error.warning(null, "%d lost pixels", lostPixels);
        }

        Statistics.instance().potential.averageDirectPotential = Statistics.instance().potential.totalDirectPotential =
            Statistics.instance().potential.maxDirectPotential = Statistics.instance().potential.maxDirectImportance = 0.0;
        for (int i = 1; i <= maximumPatchId; i++) {
            Patch patch = id2patch[i];

            if (patch != null) {
                patch.directPotential = newDirectImportance[i] / patch.area;

                if (patch.directPotential > Statistics.instance().potential.maxDirectPotential) {
                    Statistics.instance().potential.maxDirectPotential = patch.directPotential;
                }
                Statistics.instance().potential.totalDirectPotential += newDirectImportance[i];
                Statistics.instance().potential.averageDirectPotential += newDirectImportance[i];

                if (newDirectImportance[i] > Statistics.instance().potential.maxDirectImportance) {
                    Statistics.instance().potential.maxDirectImportance = newDirectImportance[i];
                }
            }
        }
        Statistics.instance().potential.averageDirectPotential /= Statistics.instance().radiance.totalArea;
    }

    private static void softGetPatchPointers(SglContext sgl, ArrayList<Patch> scenePatches) {
        for (int i = 0; scenePatches != null && i < scenePatches.size(); i++) {
            scenePatches.get(i).setInvisible();
        }

        int pixelCount = sgl.width * sgl.height;
        for (int i = 0; i < pixelCount; i++) {
            Patch P = sgl.patchBuffer[i];
            if (P != null) {
                P.setVisible();
            }
        }
    }

    private static void softUpdateDirectVisibility(Scene scene, RenderOptions renderOptions) {
        long t = System.nanoTime();
        SglContext currentSglContext = SoftIds.setupSoftFrameBuffer(scene.camera);

        SoftIds.softRenderPatches(scene, renderOptions, currentSglContext);
        Potential.softGetPatchPointers(currentSglContext, scene.patchList);

        System.err.printf("Determining visible patches in software took %g sec\n",
            (float)((double)(System.nanoTime() - t) / 1000000000.0));
    }

    /**
Updates view visibility status of all patches
*/
    public static void updateDirectVisibility(Scene scene, RenderOptions renderOptions) {
        Canvas.canvasPushMode();
        Potential.softUpdateDirectVisibility(scene, renderOptions);
        Canvas.canvasPullMode();
    }
}
