/**
Many routines borrowed from Density Estimation master thesis by Olivier Ceulemans.
*/

package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.linealAlgebra.Vector2Df;
import vsdk.toolkit.render.ScreenBuffer;

public class Kernel2D {
    private static final float M_2_PI = (float)(2.0 / Math.PI);

    private float m_h; // Kernel size.
    private float m_h2; // h2=h*h.
    private float m_h2inv; // h2inv=1/h2.
    private float m_weight; // The weight of the kernel

    public Kernel2D() {
        Init(1.0f, 1.0f);
    }

    public void Init(float h, float w) {
        m_h = h;
        m_weight = w;

        m_h2 = h * h;
        m_h2inv = 1.0f / m_h2;
    }

    /**
    Change kernel width.
    IN:   The new kernel width, newH.
    PRE:  newH > 0.0f
    POST: The kernel size is newH.
    */
    public void SetH(float newH) {
        Init(newH, m_weight);
    }

    /**
    Evaluate the kernel
    IN:  The position of a 2D point.
         The position of the center of the kernel.
    OUT: The value of the kernel at the point. (a positive number or 0.0f)
    */
    public float Evaluate(Vector2Df point, Vector2Df center) {
        // Find distance
        float tp = point.subtract(center).lengthSquared();

        if ( tp < m_h2 ) {
            // Point inside kernel
            tp = (1.0f - (tp * m_h2inv));
            tp = M_2_PI * tp * m_h2inv;
            return tp;
        } else {
            return 0.0f;
        }
    }

    /**
    cover the affected pixels of a screen buffer with the kernel
    IN: The screen buffer to cover.
    OUT: /
    */
    public void cover(Vector2Df point, float scale, ColorRgbMutable col, ScreenBuffer screen) {
        // For each neighbourhood pixel : eval kernel and add contrib

        int[] nxMinV = new int[1];
        int[] nxMaxV = new int[1];
        int[] nyMinV = new int[1];
        int[] nyMaxV = new int[1];
        Vector2Df center;
        ColorRgbMutable addCol = new ColorRgbMutable();
        float factor;

        // Get extents of possible pixels that are affected
        screen.getPixel(point.x() - m_h, point.y() - m_h, nxMinV, nyMinV);
        screen.getPixel(point.x() + m_h, point.y() + m_h, nxMaxV, nyMaxV);

        int nxMin = nxMinV[0];
        int nxMax = nxMaxV[0];
        int nyMin = nyMinV[0];
        int nyMax = nyMaxV[0];

        for ( int nx = nxMin; nx <= nxMax; nx++ ) {
            for ( int ny = nyMin; ny <= nyMax; ny++ ) {
                if ( (nx >= 0) && (ny >= 0) && (nx < screen.getHRes()) && (ny < screen.getVRes()) ) {
                    center = screen.getPixelCenter(nx, ny);
                    factor = scale * Evaluate(point, center);
                    addCol.scaledCopy(factor, new ColorRgbMutable(col));
                    screen.add(nx, ny, new ColorRgbMutable(addCol));
                } else {
                    // Handle boundary bias !
                }
            }
        }
    }

    /**
    Add one hit/splat with a size dependend on a reference estimate
    */
    public void
    varCover(
        Vector2Df center,
        ColorRgbMutable color,
        ScreenBuffer ref,
        ScreenBuffer dest,
        int totalSamples,
        int scaleSamples,
        float baseSize)
    {
        float screenScale = Math.max(ref.getPixXSize(), ref.getPixYSize());
        float B = baseSize * screenScale; // what about the 8 ??

        // Use optimal N (samples per pixel) dependency for fixed kernels
        // scaleSamples is normally total samples per pixel, while
        // totalSamples is the total number of samples for the CURRENT
        // number of samples per pixel
        float Bn = (float)(B * (Math.pow((double)scaleSamples, (-1.5 / 5.0))));

        float h;

        // Now compute h for this sample

        // Reference estimated function
        ColorRgbMutable fe = ref.getBiLinear(center.x(), center.y());

        float avgFe = (float) fe.average();
        float avgG = (float) color.average();

        if ( avgFe > VSDK.EPSILON ) {
            h = (float)(Bn * Math.sqrt(avgG / avgFe));
            // printf("fe %f G %f, h = %f\n", avgFe, avgG, h/screenScale);
        } else {
            final float maxRatio = 20; // ???
            h = Bn * maxRatio * screenScale;

            System.out.printf("MaxRatio... h = %f\n", h / screenScale);
        }

        h = Math.max(1.0f * screenScale, h); // We want to cover at least one pixel...

        SetH(h);

        // h determined, now splat the fucker
        cover(center, 1.0f / (float)totalSamples, color, dest);
    }
}
