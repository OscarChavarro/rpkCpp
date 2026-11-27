#ifndef __POLYGON_VERTEX__
#define __POLYGON_VERTEX__

/**
Immutable polygon vertex: homogeneous screen space position (sx, sy, sz, sw),
world space position (x, y, z), texture position (u, v) and color (r, g, b).
Each attribute also has an index, in that order, to select it by number in
the interpolation masks of ScreenPolygon and in getCoord.
*/
class PolygonVertex {
  public:
    enum {
        SX = 0,
        SY = 1,
        SZ = 2,
        SW = 3,
        X = 4,
        Y = 5,
        Z = 6,
        U = 7,
        V = 8,
        R = 9,
        G = 10,
        B = 11,
        NUMBER_OF_ATTRIBUTES = 12
    };

  private:
    double attributes[NUMBER_OF_ATTRIBUTES];

  public:
    PolygonVertex()
    {
        int i;
        for ( i = 0; i < NUMBER_OF_ATTRIBUTES; i++ ) {
            attributes[i] = 0.0;
        }
    }

    PolygonVertex(
        double inSx, double inSy, double inSz, double inSw,
        double inX, double inY, double inZ,
        double inU, double inV,
        double inR, double inG, double inB)
    {
        attributes[SX] = inSx;
        attributes[SY] = inSy;
        attributes[SZ] = inSz;
        attributes[SW] = inSw;
        attributes[X] = inX;
        attributes[Y] = inY;
        attributes[Z] = inZ;
        attributes[U] = inU;
        attributes[V] = inV;
        attributes[R] = inR;
        attributes[G] = inG;
        attributes[B] = inB;
    }

    /** Vertex with the NUMBER_OF_ATTRIBUTES attributes given in index order */
    static PolygonVertex
    fromAttributes(const double *values) {
        return PolygonVertex(values[SX], values[SY], values[SZ], values[SW],
                             values[X], values[Y], values[Z],
                             values[U], values[V],
                             values[R], values[G], values[B]);
    }

    double getSx() const { return attributes[SX]; }
    double getSy() const { return attributes[SY]; }
    double getSz() const { return attributes[SZ]; }
    double getSw() const { return attributes[SW]; }
    double getX() const { return attributes[X]; }
    double getY() const { return attributes[Y]; }
    double getZ() const { return attributes[Z]; }
    double getU() const { return attributes[U]; }
    double getV() const { return attributes[V]; }
    double getR() const { return attributes[R]; }
    double getG() const { return attributes[G]; }
    double getB() const { return attributes[B]; }

    /** The attribute of the given index, or 0.0 outside [0, NUMBER_OF_ATTRIBUTES) */
    double
    getCoord(int index) const {
        return index >= 0 && index < NUMBER_OF_ATTRIBUTES ? attributes[index] : 0.0;
    }
};

#endif
