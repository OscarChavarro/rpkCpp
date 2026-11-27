#ifndef __RASTER_RECTANGLE_2D__
#define __RASTER_RECTANGLE_2D__

/**
Immutable integer screen rectangle. The minimum corner (minX, minY) and the
maximum corner (maxX, maxY) are both inclusive. Instances offer no mutators: a
new value is obtained by constructing another rectangle.
*/
class RasterRectangle2D final {
  private:
    int minX;
    int minY;
    int maxX;
    int maxY;

  public:
    constexpr RasterRectangle2D(): minX(0), minY(0), maxX(0), maxY(0) {}
    constexpr RasterRectangle2D(int inMinX, int inMinY, int inMaxX, int inMaxY): minX(inMinX), minY(inMinY), maxX(inMaxX), maxY(inMaxY) {}

    constexpr int getMinX() const { return minX; }
    constexpr int getMinY() const { return minY; }
    constexpr int getMaxX() const { return maxX; }
    constexpr int getMaxY() const { return maxY; }
};

#endif
