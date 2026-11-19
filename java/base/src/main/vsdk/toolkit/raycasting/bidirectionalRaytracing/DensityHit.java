package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.color.ColorRgbMutable;

/**
Class DensityBuffer : class for storing sample hits on screen
New samples are added with 'Add'. 'reconstruct' reconstructs
an approximation to the sampled function into a screen buffer
*/
public class DensityHit {
    public float m_x; // Screen/Polygon Coordinates
    public float m_y;
    public ColorRgbMutable color; // Estimate of the function, NOT divided by number of samples

    public DensityHit() {
        color = new ColorRgbMutable();
    }

    public void init(float x, float y, ColorRgbMutable col) {
        m_x = x;
        m_y = y;
        color = new ColorRgbMutable(col.getR(), col.getG(), col.getB());
    }

    public DensityHit(float x, float y, ColorRgbMutable col) {
        this();
        init(x, y, col);
    }
}
