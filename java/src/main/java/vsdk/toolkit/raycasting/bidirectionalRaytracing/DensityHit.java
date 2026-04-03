package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.ColorRgb;

/**
Class DensityBuffer : class for storing sample hits on screen
New samples are added with 'Add'. 'reconstruct' reconstructs
an approximation to the sampled function into a screen buffer
*/
public class DensityHit {
    public float m_x; // Screen/Polygon Coordinates
    public float m_y;
    public ColorRgb color; // Estimate of the function, NOT divided by number of samples

    public DensityHit() {
        color = new ColorRgb();
    }

    public void init(float x, float y, ColorRgb col) {
        m_x = x;
        m_y = y;
        color = new ColorRgb(col.r, col.g, col.b);
    }

    public DensityHit(float x, float y, ColorRgb col) {
        this();
        init(x, y, col);
    }
}
