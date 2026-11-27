package vsdk.toolkit.render.sgl;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.linealAlgebra.Vector4Dd;

/** Projection, viewport and optional depth plane shared by typed SGL targets. */
public class SglContext {
    private final int width;
    private final int height;
    private Window viewport;
    private Matrix4x4d transform;
    private final long[] depthBuffer;

    public SglContext(int inWidth, int inHeight, boolean depthTesting) {
        width = inWidth;
        height = inHeight;
        viewport = window(0, 0, inWidth - 1, inHeight - 1);
        transform = new Matrix4x4d();
        depthBuffer = depthTesting ? new long[inWidth * inHeight] : null;
    }

    private static Window window(int x0, int y0, int x1, int y1) {
        Window result = new Window();
        result.x0 = x0;
        result.y0 = y0;
        result.x1 = x1;
        result.y1 = y1;
        return result;
    }

    public int getWidth() {
        return width;
    }

    public int getHeight() {
        return height;
    }

    public int getViewportWidth() {
        return viewport.x1 - viewport.x0 + 1;
    }

    public int getViewportHeight() {
        return viewport.y1 - viewport.y0 + 1;
    }

    public Window getViewport() {
        return viewport;
    }

    public long[] getDepthBuffer() {
        return depthBuffer;
    }

    public void setViewport(int x, int y, int viewportWidth, int viewportHeight) {
        if ( x < 0 || y < 0 || viewportWidth <= 0 || viewportHeight <= 0 ||
             viewportWidth > width - x || viewportHeight > height - y ) {
            Logger.reportMessage(this, Logger.ERROR, "setViewport", "Viewport lies outside the raster target");
            return;
        }

        viewport = window(x, y, x + viewportWidth - 1, y + viewportHeight - 1);
    }

    public void setTransform(Matrix4x4d matrix) {
        transform = matrix;
    }

    public boolean projectPolygon(int numberOfVertices, Vector3Dd[] vertices, ScreenPolygon pol) {
        PolygonBox clipBox = new PolygonBox(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

        if ( numberOfVertices < 3 || numberOfVertices > PolygonClipResultInfo.MAXIMUM_SIDES_PER_POLYGON - 6 ) {
            Logger.reportMessage(this, Logger.ERROR, "projectPolygon", "Invalid vertex count for polygon rasterization");
            return false;
        }

        // Transform the vertices and fill in a Poly
        for ( int i = 0; i < numberOfVertices; i++ ) {
            PolygonVertex vertex = pol.vertices[i];
            final Vector4Dd v = transform.multiply(
                new Vector4Dd(vertices[i].x(), vertices[i].y(), vertices[i].z(), 1.0));
            if ( v.w() > -VSDK.EPSILON && v.w() < VSDK.EPSILON ) {
                return false;
            }
            vertex.sx = v.x();
            vertex.sy = v.y();
            vertex.sz = v.z();
            vertex.sw = v.w();
        }
        pol.n = numberOfVertices;
        pol.mask = Poly.mask(0L) |
            Poly.mask(Double.BYTES) |
            Poly.mask(2L * Double.BYTES) |
            Poly.mask(3L * Double.BYTES);
        if ( Poly.clipToBox(pol, clipBox) == PolygonClipResult.POLY_CLIP_OUT ) {
            return false;
        }

        // Perspective divide and transformation to viewport and depth range
        for ( int i = 0; i < pol.n; i++ ) {
            PolygonVertex vertex = pol.vertices[i];
            vertex.sx = (double)viewport.x0 + (vertex.sx / vertex.sw + 1.0) * (double)getViewportWidth() * 0.5;
            vertex.sy = (double)viewport.y0 + (vertex.sy / vertex.sw + 1.0) * (double)getViewportHeight() * 0.5;
            vertex.sz = ((vertex.sz / vertex.sw + 1.0) * 0.5) * (double)SglConstants.SGL_MAXIMUM_Z;
        }

        return true;
    }
}
