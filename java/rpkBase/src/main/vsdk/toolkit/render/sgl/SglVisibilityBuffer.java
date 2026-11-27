package vsdk.toolkit.render.sgl;

import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

/** A single payload per sample. Depth, when enabled, belongs to this pass. */
public class SglVisibilityBuffer<Sample> {
    private final SglContext context;
    private final Sample background;
    private final Sample[] samples;

    public SglVisibilityBuffer(int width, int height, Sample backgroundValue) {
        this(width, height, backgroundValue, true);
    }

    @SuppressWarnings("unchecked")
    public SglVisibilityBuffer(int width, int height, Sample backgroundValue, boolean depthTesting) {
        context = new SglContext(width, height, depthTesting);
        background = backgroundValue;
        samples = (Sample[])new Object[width * height];
        clear();
    }

    public int getWidth() {
        return context.getWidth();
    }

    public int getHeight() {
        return context.getHeight();
    }

    public Window getViewport() {
        return context.getViewport();
    }

    public int getViewportWidth() {
        return context.getViewportWidth();
    }

    public int getViewportHeight() {
        return context.getViewportHeight();
    }

    public Sample getSample(int x, int y) {
        return samples[y * getWidth() + x];
    }

    public void setViewport(int x, int y, int width, int height) {
        context.setViewport(x, y, width, height);
    }

    public void setTransform(Matrix4x4d matrix) {
        context.setTransform(matrix);
    }

    public void clear() {
        final Window viewport = getViewport();
        final long[] depth = context.getDepthBuffer();
        for ( int y = viewport.y0; y <= viewport.y1; y++ ) {
            final int firstSample = y * getWidth() + viewport.x0;
            final int endSample = firstSample + getViewportWidth();
            for ( int i = firstSample; i < endSample; i++ ) {
                samples[i] = background;
            }
            if ( depth != null ) {
                for ( int i = firstSample; i < endSample; i++ ) {
                    depth[i] = SglConstants.SGL_MAXIMUM_Z;
                }
            }
        }
    }

    public void drawPolygon(int numberOfVertices, Vector3Dd[] vertices, Sample value) {
        ScreenPolygon polygon = new ScreenPolygon();
        if ( !context.projectPolygon(numberOfVertices, vertices, polygon) ) {
            return;
        }

        final long[] depth = context.getDepthBuffer();
        if ( depth != null ) {
            Poly.scanZ(samples, depth, getWidth(), value, polygon, getViewport());
        } else {
            Poly.scanFlat(samples, getWidth(), value, polygon, getViewport());
        }
    }
}
