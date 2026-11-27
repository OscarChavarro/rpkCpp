#ifndef __POLYGON_BOX__
#define __POLYGON_BOX__

/** Immutable box, typically in screen space */
class PolygonBox {
  private:
    double minX; // Left and right
    double maxX;
    double minY; // Top and bottom
    double maxY;
    double minZ; // Near and far
    double maxZ;

  public:
    PolygonBox(double inMinX, double inMaxX, double inMinY, double inMaxY, double inMinZ, double inMaxZ):
        minX(inMinX), maxX(inMaxX), minY(inMinY), maxY(inMaxY), minZ(inMinZ), maxZ(inMaxZ) {}

    double getMinX() const { return minX; }
    double getMaxX() const { return maxX; }
    double getMinY() const { return minY; }
    double getMaxY() const { return maxY; }
    double getMinZ() const { return minZ; }
    double getMaxZ() const { return maxZ; }
};

#endif
