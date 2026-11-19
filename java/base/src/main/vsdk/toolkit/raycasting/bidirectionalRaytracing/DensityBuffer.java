/**
Density estimation on screen
*/

package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector2D;
import vsdk.toolkit.render.ScreenBuffer;

public class DensityBuffer {
    private static final int DHA_X_RES = 50;
    private static final int DHA_Y_RES = 50; // Subdivide image plane for efficient hit searches

    // A matching screen buffer is kept. This one will be filled in
    // by density estimation...
    private ScreenBuffer screenBuffer;
    private BidirectionalPathRaytracerConfig baseConfig;
    private float xMinimum; // Copy from screenBuffer
    private float xMaximum;
    private float yMinimum;
    private float yMaximum;
    private DensityHitList[][] hitGrid = new DensityHitList[DHA_X_RES][DHA_Y_RES];

    private int xIndex(float x) {
        return Math.min((int)(DensityBuffer.DHA_X_RES * (x - xMinimum) / (xMaximum - xMinimum)), DensityBuffer.DHA_X_RES - 1);
    }

    private int yIndex(float y) {
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
    public void add(float x, float y, ColorRgb color) {
        float factor = screenBuffer.getPixXSize() * screenBuffer.getPixYSize()
            * (float)baseConfig.totalSamples;
        ColorRgbMutable tmpCol = new ColorRgbMutable();

        if ( color.average() > Numeric.EPSILON ) {
            tmpCol.scaledCopy(factor, new ColorRgbMutable(color)); // Undo part of flux to rad factor

            DensityHit hit = new DensityHit(x, y, tmpCol.toImmutable());

            hitGrid[xIndex(x)][yIndex(y)].add(hit);
        }
    }

    /**
    Reconstruct the internal screen buffer using constant kernel width
    */
    public ScreenBuffer reconstruct() {
        // For all samples -> compute pixel coverage

        // Kernel size. Now spread over 3 pixels
        float h = (float)(8.0 * Math.max(screenBuffer.getPixXSize(), screenBuffer.getPixYSize())
            / Math.sqrt((float)baseConfig.samplesPerPixel));

        System.out.printf("h = %f\n", h);

        screenBuffer.scaleRadiance(0.0f); // Hack!

        int maxK;
        DensityHit hit;
        Kernel2D kernel = new Kernel2D();
        Vector2D center = new Vector2D();

        kernel.SetH(h);

        for ( int i = 0; i < DensityBuffer.DHA_X_RES; i++ ) {
            for ( int j = 0; j < DensityBuffer.DHA_Y_RES; j++ ) {
                maxK = hitGrid[i][j].storedHits();

                for ( int k = 0; k < maxK; k++ ) {
                    hit = hitGrid[i][j].get(k);

                    center.x = hit.m_x;
                    center.y = hit.m_y;

                    kernel.cover(center, 1.0f / (float)baseConfig.totalSamples, hit.color, screenBuffer);
                }
            }
        }

        return screenBuffer;
    }


    public ScreenBuffer reconstructVariable(ScreenBuffer dest, float baseSize) {
        // For all samples -> compute pixel coverage

        // Base Kernel size. Now spread over a number of pixels

        dest.scaleRadiance(0.0f); // Hack!

        int maxK;
        DensityHit hit;
        Kernel2D kernel = new Kernel2D();
        Vector2D center = new Vector2D();

        for ( int i = 0; i < DensityBuffer.DHA_X_RES; i++ ) {
            for ( int j = 0; j < DensityBuffer.DHA_Y_RES; j++ ) {
                maxK = hitGrid[i][j].storedHits();

                for ( int k = 0; k < maxK; k++ ) {
                    hit = hitGrid[i][j].get(k);

                    center.x = hit.m_x;
                    center.y = hit.m_y;

                    kernel.varCover(center, hit.color, screenBuffer, dest, (int)baseConfig.totalSamples,
                        baseConfig.samplesPerPixel, baseSize);
                }
            }
        }

        return dest;
    }

    public ScreenBuffer reconstructVariable(ScreenBuffer dest) {
        return reconstructVariable(dest, 4.0f);
    }
}
