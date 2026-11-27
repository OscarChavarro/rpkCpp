#ifndef __POLYGON_BOX__
#define __POLYGON_BOX__

/** Immutable box, typically in screen space */
class PolygonBox final {
  private:
    double minX; // Left and right
    double maxX;
    double minY; // Top and bottom
    double maxY;
    double minZ; // Near and far
    double maxZ;

  public:
    constexpr PolygonBox(double inMinX, double inMaxX, double inMinY, double inMaxY, double inMinZ, double inMaxZ):
        minX(inMinX), maxX(inMaxX), minY(inMinY), maxY(inMaxY), minZ(inMinZ), maxZ(inMaxZ) {}

    constexpr double getMinX() const { return minX; }
    constexpr double getMaxX() const { return maxX; }
    constexpr double getMinY() const { return minY; }
    constexpr double getMaxY() const { return maxY; }
    constexpr double getMinZ() const { return minZ; }
    constexpr double getMaxZ() const { return maxZ; }
};

#endif
