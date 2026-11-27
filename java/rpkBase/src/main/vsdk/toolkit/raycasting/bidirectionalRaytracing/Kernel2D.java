/**
Many routines borrowed from Density Estimation master thesis by Olivier Ceulemans.
*/

package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.render.ScreenBuffer;

public class Kernel2D {
    private static final double M_2_PI = (double)(2.0 / Math.PI);

    private double m_h; // Kernel size.
    private double m_h2; // h2=h*h.
    private double m_h2inv; // h2inv=1/h2.
    private double m_weight; // The weight of the kernel

    public Kernel2D() {
        Init(1.0, 1.0);
    }

    public void Init(double h, double w) {
        m_h = h;
        m_weight = w;

        m_h2 = h * h;
        m_h2inv = 1.0 / m_h2;
    }

    /**
    Change kernel width.
    IN:   The new kernel width, newH.
    PRE:  newH > 0.0
    POST: The kernel size is newH.
    */
    public void SetH(double newH) {
        Init(newH, m_weight);
    }

    /**
    Evaluate the kernel
    IN:  The position of a 2D point.
         The position of the center of the kernel.
    OUT: The value of the kernel at the point. (a positive number or 0.0)
    */
    public double Evaluate(Vector2Dd point, Vector2Dd center) {
        // Find distance
        double tp = point.subtract(center).lengthSquared();

        if ( tp < m_h2 ) {
            // Point inside kernel
            tp = (1.0 - (tp * m_h2inv));
            tp = M_2_PI * tp * m_h2inv;
            return tp;
        } else {
            return 0.0;
        }
    }

    /**
    cover the affected pixels of a screen buffer with the kernel
    IN: The screen buffer to cover.
    OUT: /
    */
    public void cover(Vector2Dd point, double scale, ColorRgb col, ScreenBuffer screen) {
        // For each neighbourhood pixel : eval kernel and add contrib

        int[] nxMinV = new int[1];
        int[] nxMaxV = new int[1];
        int[] nyMinV = new int[1];
        int[] nyMaxV = new int[1];
        Vector2Dd center;
        ColorRgb addCol = new ColorRgb();
        double factor;

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
                    addCol = (new ColorRgb(col)).multiply(factor);
                    screen.add(nx, ny, new ColorRgb(addCol));
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
        Vector2Dd center,
        ColorRgb color,
        ScreenBuffer ref,
        ScreenBuffer dest,
        int totalSamples,
        int scaleSamples,
        double baseSize)
    {
        double screenScale = Math.max(ref.getPixXSize(), ref.getPixYSize());
        double B = baseSize * screenScale; // what about the 8 ??

        // Use optimal N (samples per pixel) dependency for fixed kernels
        // scaleSamples is normally total samples per pixel, while
        // totalSamples is the total number of samples for the CURRENT
        // number of samples per pixel
        double Bn = (double)(B * (Math.pow((double)scaleSamples, (-1.5 / 5.0))));

        double h;

        // Now compute h for this sample

        // Reference estimated function
        ColorRgb fe = ref.getBiLinear(center.x(), center.y());

        double avgFe = (double)fe.average();
        double avgG = (double)color.average();

        if ( avgFe > VSDK.EPSILON ) {
            h = (double)(Bn * Math.sqrt(avgG / avgFe));
            // printf("fe %f G %f, h = %f\n", avgFe, avgG, h/screenScale);
        } else {
            final double maxRatio = 20; // ???
            h = Bn * maxRatio * screenScale;

            System.out.printf("MaxRatio... h = %f\n", h / screenScale);
        }

        h = Math.max(1.0 * screenScale, h); // We want to cover at least one pixel...

        SetH(h);

        // h determined, now splat the fucker
        cover(center, 1.0 / (double)totalSamples, color, dest);
    }
}
