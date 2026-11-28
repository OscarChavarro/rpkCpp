package vsdk.toolkit.render.raster.polygonScanConverter;

/**
Immutable integer screen rectangle. The minimum corner (minX, minY) and the
maximum corner (maxX, maxY) are both inclusive. Instances offer no mutators: a
new value is obtained by constructing another rectangle.
*/
public final class RasterRectangle2D {
    private final int minX;
    private final int minY;
    private final int maxX;
    private final int maxY;

    public RasterRectangle2D() {
        this(0, 0, 0, 0);
    }

    public RasterRectangle2D(int inMinX, int inMinY, int inMaxX, int inMaxY) {
        minX = inMinX;
        minY = inMinY;
        maxX = inMaxX;
        maxY = inMaxY;
    }

    public int getMinX() { return minX; }
    public int getMinY() { return minY; }
    public int getMaxX() { return maxX; }
    public int getMaxY() { return maxY; }
}
