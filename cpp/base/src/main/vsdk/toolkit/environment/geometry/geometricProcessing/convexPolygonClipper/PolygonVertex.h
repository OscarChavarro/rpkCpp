#ifndef __POLYGON_VERTEX__
#define __POLYGON_VERTEX__

/**
Immutable polygon vertex: homogeneous screen space position (sx, sy, sz, sw),
world space position (x, y, z), texture position (u, v) and color (r, g, b).
Each attribute also has an index, in that order, to select it by number in
the interpolation masks of ScreenPolygon and in getCoord.
*/
class PolygonVertex final {
  public:
    static constexpr int SX = 0;
    static constexpr int SY = 1;
    static constexpr int SZ = 2;
    static constexpr int SW = 3;
    static constexpr int X = 4;
    static constexpr int Y = 5;
    static constexpr int Z = 6;
    static constexpr int U = 7;
    static constexpr int V = 8;
    static constexpr int R = 9;
    static constexpr int G = 10;
    static constexpr int B = 11;
    static constexpr int NUMBER_OF_ATTRIBUTES = 12;

  private:
    double attributes[NUMBER_OF_ATTRIBUTES];

  public:
    constexpr PolygonVertex(): attributes{} {}

    constexpr PolygonVertex(
        double inSx, double inSy, double inSz, double inSw,
        double inX, double inY, double inZ,
        double inU, double inV,
        double inR, double inG, double inB):
        attributes{inSx, inSy, inSz, inSw, inX, inY, inZ, inU, inV, inR, inG, inB} {}

    /** Vertex with the NUMBER_OF_ATTRIBUTES attributes given in index order */
    static constexpr PolygonVertex
    fromAttributes(const double *values) {
        return PolygonVertex(values[SX], values[SY], values[SZ], values[SW],
                             values[X], values[Y], values[Z],
                             values[U], values[V],
                             values[R], values[G], values[B]);
    }

    constexpr double getSx() const { return attributes[SX]; }
    constexpr double getSy() const { return attributes[SY]; }
    constexpr double getSz() const { return attributes[SZ]; }
    constexpr double getSw() const { return attributes[SW]; }
    constexpr double getX() const { return attributes[X]; }
    constexpr double getY() const { return attributes[Y]; }
    constexpr double getZ() const { return attributes[Z]; }
    constexpr double getU() const { return attributes[U]; }
    constexpr double getV() const { return attributes[V]; }
    constexpr double getR() const { return attributes[R]; }
    constexpr double getG() const { return attributes[G]; }
    constexpr double getB() const { return attributes[B]; }

    /** The attribute of the given index, or 0.0 outside [0, NUMBER_OF_ATTRIBUTES) */
    constexpr double
    getCoord(int index) const {
        return index >= 0 && index < NUMBER_OF_ATTRIBUTES ? attributes[index] : 0.0;
    }
};

#endif
