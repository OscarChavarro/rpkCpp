package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.color.ColorRgb;

/**
Class DensityBuffer : class for storing sample hits on screen
New samples are added with 'Add'. 'reconstruct' reconstructs
an approximation to the sampled function into a screen buffer
*/
public class DensityHit {
    public double m_x; // Screen/Polygon Coordinates
    public double m_y;
    public ColorRgb color; // Estimate of the function, NOT divided by number of samples

    public DensityHit() {
        color = new ColorRgb();
    }

    public void init(double x, double y, ColorRgb col) {
        m_x = x;
        m_y = y;
        color = new ColorRgb(col.getR(), col.getG(), col.getB());
    }

    public DensityHit(double x, double y, ColorRgb col) {
        this();
        init(x, y, col);
    }
}
