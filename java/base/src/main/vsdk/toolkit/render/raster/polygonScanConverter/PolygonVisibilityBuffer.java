package vsdk.toolkit.render.raster.polygonScanConverter;

import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.geometricProcessing.convexPolygonClipper.ScreenPolygon;

/**
A single payload per sample. Depth, when enabled, belongs to this pass.

Implementation of PolygonVisibilityBuffer, generic on its Sample payload. It
does not know which payloads exist: each client module that defines one
instantiates it with its own Sample type. That keeps the dependency pointing
from the client to this package.
*/
public class PolygonVisibilityBuffer<Sample> {
    private final PolygonRasterContext context;
    private final Sample background;
    private final Sample[] samples;

    public PolygonVisibilityBuffer(int width, int height, Sample backgroundValue) {
        this(width, height, backgroundValue, true);
    }

    @SuppressWarnings("unchecked")
    public PolygonVisibilityBuffer(int width, int height, Sample backgroundValue, boolean depthTesting) {
        context = new PolygonRasterContext(width, height, depthTesting);
        background = backgroundValue;
        // The array never leaves this class, so its erased element type is safe
        samples = (Sample[])new Object[width * height];
        clear();
    }

    public int getWidth() { return context.getWidth(); }
    public int getHeight() { return context.getHeight(); }
    public RasterRectangle2D getViewport() { return context.getViewport(); }
    public int getViewportWidth() { return context.getViewportWidth(); }
    public int getViewportHeight() { return context.getViewportHeight(); }
    public Sample getSample(int x, int y) { return samples[y * getWidth() + x]; }

    public void setViewport(int x, int y, int width, int height) { context.setViewport(x, y, width, height); }
    public void setTransform(Matrix4x4d matrix) { context.setTransform(matrix); }

    public void
    clear() {
        final RasterRectangle2D viewport = getViewport();
        final long[] depth = context.getDepthBuffer();
        for ( int y = viewport.getMinY(); y <= viewport.getMaxY(); y++ ) {
            final int firstSample = y * getWidth() + viewport.getMinX();
            final int endSample = firstSample + getViewportWidth();
            for ( int i = firstSample; i < endSample; i++ ) {
                samples[i] = background;
            }
            if ( depth != null ) {
                for ( int i = firstSample; i < endSample; i++ ) {
                    depth[i] = PolygonScanConverter.MAXIMUM_DEPTH;
                }
            }
        }
    }

    public void
    drawPolygon(int numberOfVertices, Vector3Dd[] vertices, Sample value) {
        final ScreenPolygon polygon = context.projectPolygon(numberOfVertices, vertices);
        if ( polygon.getNumberOfVertices() == 0 ) {
            return;
        }

        final long[] depth = context.getDepthBuffer();
        if ( depth != null ) {
            PolygonScanConverter.scanZ(samples, depth, getWidth(), value, polygon, getViewport());
        } else {
            PolygonScanConverter.scanFlat(samples, getWidth(), value, polygon, getViewport());
        }
    }
}
