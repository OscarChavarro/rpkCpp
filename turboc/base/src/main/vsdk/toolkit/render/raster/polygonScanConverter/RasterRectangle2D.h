#ifndef __RASTER_RECTANGLE_2D__
#define __RASTER_RECTANGLE_2D__

/**
Immutable integer screen rectangle. The minimum corner (minX, minY) and the
maximum corner (maxX, maxY) are both inclusive. Instances offer no mutators: a
new value is obtained by constructing another rectangle.
*/
class RasterRectangle2D {
  private:
    int minX;
    int minY;
    int maxX;
    int maxY;

  public:
    RasterRectangle2D(): minX(0), minY(0), maxX(0), maxY(0) {}
    RasterRectangle2D(int inMinX, int inMinY, int inMaxX, int inMaxY): minX(inMinX), minY(inMinY), maxX(inMaxX), maxY(inMaxY) {}

    int getMinX() const { return minX; }
    int getMinY() const { return minY; }
    int getMaxX() const { return maxX; }
    int getMaxY() const { return maxY; }
};

#endif
