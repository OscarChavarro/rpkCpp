/**
Routines dealing with view potential
*/

package vsdk.toolkit.render;

import vsdk.toolkit.scene.FaceVisibilityTable;
import vsdk.toolkit.scene.DirectPotentialTable;
import java.util.ArrayList;

import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.environment.geometry.elements.Patch;

/**
In analogy with [SMIT1992] Smits, "Importance-driven Radiosity", SIGGRAPH '92, we
call the integral of potential over surface area "importance"
*/
public class Potential {
    /**
Updates directly received potential for all patches
*/
    public static void updateDirectPotential(Scene scene, RendererConfiguration renderOptions) {
        Canvas.canvasPushMode();

        FaceVisibilityBuffer target = new FaceVisibilityBuffer(scene.camera.xSize, scene.camera.ySize, FaceIndexSample.background());
        SoftIds.renderVisibleFaces(scene, renderOptions, target);
        long[] x = {target.getWidth()};
        long[] y = {target.getHeight()};

        Canvas.canvasPullMode();

        long lostPixels = 0;

        // Build a table to convert a patch ID to the corresponding Patch
        int maximumPatchId = Patch.getNextId() - 1;
        Patch[] id2patch = new Patch[maximumPatchId + 1];
        for (int i = 0; i <= maximumPatchId; i++) {
            id2patch[i] = null;
        }
        for (int i = 0; scene.patchList != null && i < scene.patchList.size(); i++) {
            Patch patch = scene.patchList.get(i);
            id2patch[patch.getId()] = patch;
        }

        // Allocate space for an array to hold the new direct potential of the patches
        double[] newDirectImportance = new double[maximumPatchId + 1];
        for (int i = 0; i <= maximumPatchId; i++) {
            newDirectImportance[i] = 0.0;
        }

        // h and v are the horizontal resp. vertical distance between two
        // neighboring pixels on the screen
        double h = 2.0 * (double)Math.tan(scene.camera.horizontalFov * (double)Math.PI / 180.0) / (double)x[0];
        double v = 2.0 * (double)Math.tan(scene.camera.verticalFov * (double)Math.PI / 180.0) / (double)y[0];
        double pixelArea = h * v;

        double ySample;
        long j;
        for (j = y[0] - 1, ySample = -v * (double)(y[0] - 1) / 2.0;
             j >= 0;
             j--, ySample += v) {
            double xSample = -h * (double)(x[0] - 1) / 2.0;
            for (long i = 0; i < x[0]; i++, xSample += h) {
                final FaceIndexSample face = target.getSample((int)i, (int)j);
                long the_id = face.isBackground() ? 0 :
                    scene.getFace(face.getIndex()).getId() & 0xffffffL;

                if (the_id > 0 && the_id <= maximumPatchId) {
                    Vector3Dd pixDir = new Vector3Dd();

                    // Compute direction to center of pixel
                    pixDir = scene.camera.Z.add(scene.camera.X.multiply((double)xSample)).add(scene.camera.Y.multiply(ySample));

                    // Delta_importance = (cosine of the angle between the direction to
                    // the pixel and the viewing direction, over the distance from the
                    // eye point to the pixel) squared, times area of the pixel
                    double deltaImportance = scene.camera.Z.dotProduct(pixDir) / pixDir.dotProduct(pixDir);
                    deltaImportance *= deltaImportance * pixelArea;
                    newDirectImportance[(int)the_id] += deltaImportance;
                }
                else if (the_id > maximumPatchId) {
                    lostPixels++;
                }
            }
        }

        if (lostPixels > 0) {
            Logger.reportMessage(null, Logger.WARNING, "", String.format("%d lost pixels", lostPixels));
        }

        Statistics.instance().potential.averageDirectPotential = Statistics.instance().potential.totalDirectPotential =
            Statistics.instance().potential.maxDirectPotential = Statistics.instance().potential.maxDirectImportance = 0.0;
        for (int i = 1; i <= maximumPatchId; i++) {
            Patch patch = id2patch[i];

            if (patch != null) {
                DirectPotentialTable directPotentials = scene.getDirectPotentials();
                final int faceIndex = patch.getFaceIndex();
                directPotentials.set(faceIndex, newDirectImportance[i] / patch.getArea());

                if (directPotentials.get(faceIndex) > Statistics.instance().potential.maxDirectPotential) {
                    Statistics.instance().potential.maxDirectPotential = directPotentials.get(faceIndex);
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

    private static void softUpdateDirectVisibility(Scene scene, RendererConfiguration renderOptions) {
        long t = System.nanoTime();
        FaceVisibilityBuffer target = new FaceVisibilityBuffer(scene.camera.xSize, scene.camera.ySize, FaceIndexSample.background());
        SoftIds.renderVisibleFaces(scene, renderOptions, target);

        FaceVisibilityTable faceVisibility = scene.getFaceVisibility();
        for ( int i = 0; i < faceVisibility.getNumberOfFaces(); i++ ) {
            faceVisibility.setVisible(i, false);
        }
        for ( int y = 0; y < target.getHeight(); y++ ) {
            for ( int x = 0; x < target.getWidth(); x++ ) {
                final FaceIndexSample face = target.getSample(x, y);
                if ( !face.isBackground() ) {
                    faceVisibility.setVisible(face.getIndex(), true);
                }
            }
        }

        System.err.printf("Determining visible patches in software took %g sec\n",
            (double)((double)(System.nanoTime() - t) / 1000000000.0));
    }

    /**
Updates view visibility status of all patches
*/
    public static void updateDirectVisibility(Scene scene, RendererConfiguration renderOptions) {
        Canvas.canvasPushMode();
        Potential.softUpdateDirectVisibility(scene, renderOptions);
        Canvas.canvasPullMode();
    }
}
