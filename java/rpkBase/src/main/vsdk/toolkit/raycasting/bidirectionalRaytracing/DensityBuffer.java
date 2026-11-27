/**
Density estimation on screen
*/

package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.render.ScreenBuffer;

public class DensityBuffer {
    private static final int DHA_X_RES = 50;
    private static final int DHA_Y_RES = 50; // Subdivide image plane for efficient hit searches

    // A matching screen buffer is kept. This one will be filled in
    // by density estimation...
    private ScreenBuffer screenBuffer;
    private BidirectionalPathRaytracerConfig baseConfig;
    private double xMinimum; // Copy from screenBuffer
    private double xMaximum;
    private double yMinimum;
    private double yMaximum;
    private DensityHitList[][] hitGrid = new DensityHitList[DHA_X_RES][DHA_Y_RES];

    private int xIndex(double x) {
        return Math.min((int)(DensityBuffer.DHA_X_RES * (x - xMinimum) / (xMaximum - xMinimum)), DensityBuffer.DHA_X_RES - 1);
    }

    private int yIndex(double y) {
        return Math.min((int)(DensityBuffer.DHA_Y_RES * (y - yMinimum) / (yMaximum - yMinimum)), DensityBuffer.DHA_Y_RES - 1);
    }

    public DensityBuffer(ScreenBuffer screen, BidirectionalPathRaytracerConfig paramBaseConfig) {
        screenBuffer = screen;
        baseConfig = paramBaseConfig;

        xMinimum = screenBuffer.getScreenXMin();
        xMaximum = screenBuffer.getScreenXMax();
        yMinimum = screenBuffer.getScreenYMin();
        yMaximum = screenBuffer.getScreenYMax();

        for ( int i = 0; i < DHA_X_RES; i++ ) {
            for ( int j = 0; j < DHA_Y_RES; j++ ) {
                hitGrid[i][j] = new DensityHitList();
            }
        }

        System.out.printf("Density Buffer :\nXmin %f, Ymin %f, Xmax %f, Ymax %f\n",
            xMinimum, yMinimum, xMaximum, yMaximum);

    }

    /**
    Add a hit
    */
    public void add(double x, double y, ColorRgb color) {
        double factor = screenBuffer.getPixXSize() * screenBuffer.getPixYSize()
            * (double)baseConfig.totalSamples;
        ColorRgb tmpCol = new ColorRgb();

        if ( color.average() > VSDK.EPSILON ) {
            tmpCol = (new ColorRgb(color)).multiply(factor); // Undo part of flux to rad factor

            DensityHit hit = new DensityHit(x, y, new ColorRgb(tmpCol));

            hitGrid[xIndex(x)][yIndex(y)].add(hit);
        }
    }

    /**
    Reconstruct the internal screen buffer using constant kernel width
    */
    public ScreenBuffer reconstruct() {
        // For all samples -> compute pixel coverage

        // Kernel size. Now spread over 3 pixels
        double h = (double)(8.0 * Math.max(screenBuffer.getPixXSize(), screenBuffer.getPixYSize())
            / Math.sqrt((double)baseConfig.samplesPerPixel));

        System.out.printf("h = %f\n", h);

        screenBuffer.scaleRadiance(0.0); // Hack!

        int maxK;
        DensityHit hit;
        Kernel2D kernel = new Kernel2D();
        Vector2Dd center = new Vector2Dd();

        kernel.SetH(h);

        for ( int i = 0; i < DensityBuffer.DHA_X_RES; i++ ) {
            for ( int j = 0; j < DensityBuffer.DHA_Y_RES; j++ ) {
                maxK = hitGrid[i][j].storedHits();

                for ( int k = 0; k < maxK; k++ ) {
                    hit = hitGrid[i][j].get(k);

                    center = center.withX(hit.m_x);
                    center = center.withY(hit.m_y);

                    kernel.cover(center, 1.0 / (double)baseConfig.totalSamples, hit.color, screenBuffer);
                }
            }
        }

        return screenBuffer;
    }


    public ScreenBuffer reconstructVariable(ScreenBuffer dest, double baseSize) {
        // For all samples -> compute pixel coverage

        // Base Kernel size. Now spread over a number of pixels

        dest.scaleRadiance(0.0); // Hack!

        int maxK;
        DensityHit hit;
        Kernel2D kernel = new Kernel2D();
        Vector2Dd center = new Vector2Dd();

        for ( int i = 0; i < DensityBuffer.DHA_X_RES; i++ ) {
            for ( int j = 0; j < DensityBuffer.DHA_Y_RES; j++ ) {
                maxK = hitGrid[i][j].storedHits();

                for ( int k = 0; k < maxK; k++ ) {
                    hit = hitGrid[i][j].get(k);

                    center = center.withX(hit.m_x);
                    center = center.withY(hit.m_y);

                    kernel.varCover(center, hit.color, screenBuffer, dest, (int)baseConfig.totalSamples,
                        baseConfig.samplesPerPixel, baseSize);
                }
            }
        }

        return dest;
    }

    public ScreenBuffer reconstructVariable(ScreenBuffer dest) {
        return reconstructVariable(dest, 4.0);
    }
}
