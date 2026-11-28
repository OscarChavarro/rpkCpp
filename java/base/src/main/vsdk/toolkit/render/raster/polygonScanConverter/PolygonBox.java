package vsdk.toolkit.render.raster.polygonScanConverter;

/** Immutable box, typically in screen space */
public final class PolygonBox {
    private final double minX; // Left and right
    private final double maxX;
    private final double minY; // Top and bottom
    private final double maxY;
    private final double minZ; // Near and far
    private final double maxZ;

    public PolygonBox(double inMinX, double inMaxX, double inMinY, double inMaxY, double inMinZ, double inMaxZ) {
        minX = inMinX;
        maxX = inMaxX;
        minY = inMinY;
        maxY = inMaxY;
        minZ = inMinZ;
        maxZ = inMaxZ;
    }

    public double getMinX() { return minX; }
    public double getMaxX() { return maxX; }
    public double getMinY() { return minY; }
    public double getMaxY() { return maxY; }
    public double getMinZ() { return minZ; }
    public double getMaxZ() { return maxZ; }
}
